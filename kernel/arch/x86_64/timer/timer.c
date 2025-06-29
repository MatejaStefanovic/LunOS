#include <kernel/timer.h>
#include <kernel/klogging.h>

static void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


static inline uint64_t get_cpu_cycles(void) {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Pure CPU-based delay with calibration
static uint64_t cpu_cycles_per_10ms = 0;

// Try to use CPUID to get CPU frequency for best accuracy
static uint64_t get_cpu_frequency_mhz(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check if CPU frequency info is available
    asm volatile("cpuid" 
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                 : "a"(0x80000007));
   
    if (!(edx & (1 << 8))) 
        KWARN("TSC may still work but it isn't reliable\n");
    
    // Try to get frequency from CPUID leaf 0x15 (newer CPUs)
    asm volatile("cpuid" 
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                 : "a"(0x15));
    
    if (eax != 0 && ebx != 0 && ecx != 0) {
        // TSC frequency = (ecx * ebx) / eax
        uint64_t tsc_freq = ((uint64_t)ecx * ebx) / eax;
        return tsc_freq / 1000000;  // Convert to MHz
    }
    
    // Try CPUID leaf 0x16 (for Intel)
    asm volatile("cpuid" 
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
                 : "a"(0x16));
    
    if (eax != 0) {
        return eax;  // Frequency in MHz
    }
    
    return 0; 
}

#define RTC_SECONDS 0x00
#define RTC_REGISTER_A 0x0A
#define RTC_REGISTER_B 0x0B
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    io_wait();
    return inb(CMOS_DATA);
};

// Method 2: Use Real Time Clock, less accurate than the CPUID method but still good
static void calibrate_with_rtc(void) {
    kprintf("Calibrating with RTC...\n");
    
    // Wait for RTC update to complete
    while (read_rtc_register(RTC_REGISTER_A) & 0x80);
    
    // Get initial second
    uint8_t start_second = read_rtc_register(RTC_SECONDS);
    uint8_t current_second = start_second;
    
    // Wait for second to change (up to 1 full second)
    while (current_second == start_second) {
        current_second = read_rtc_register(RTC_SECONDS);
    }
    
    // Now measure exactly 1 second using RTC
    uint64_t start_cycles = get_cpu_cycles();
    start_second = current_second;
    
    while (current_second == start_second) {
        current_second = read_rtc_register(RTC_SECONDS);
    }
    
    uint64_t end_cycles = get_cpu_cycles();
    uint64_t cycles_per_second = end_cycles - start_cycles;
    
    // Calculate 10ms worth of cycles
    cpu_cycles_per_10ms = cycles_per_second / 100;
    
    kprintf("RTC calibration: %lu cycles/second, %lu cycles/10ms\n", 
            cycles_per_second, cpu_cycles_per_10ms);
}

static void calibrate_with_known_frequencies(void) {
    // Common CPU frequencies and their 10ms cycle counts
    struct {
        const char* description;
        uint64_t cycles_10ms;
    } known_freqs[] = {
        {"3.0 GHz CPU", 30000000},   // 3 billion cycles/sec = 30M/10ms
        {"2.4 GHz CPU", 24000000},   // Common laptop frequency
        {"3.6 GHz CPU", 36000000},   // Common desktop frequency
        {"2.0 GHz CPU", 20000000},   // Conservative estimate
        {"1.0 GHz CPU", 10000000},   // Very conservative
    };
    
    kprintf("Available frequency estimates:\n");
    for (int i = 0; i < 5; i++) {
        kprintf("%d: %s - %lu cycles per 10ms\n", 
                i, known_freqs[i].description, known_freqs[i].cycles_10ms);
    }
    
    // TODO: select automatically, right now I'm lazy so we'll just default to this
    cpu_cycles_per_10ms = 24000000;  
    kprintf("Using conservative estimate: %lu cycles per 10ms\n", cpu_cycles_per_10ms);
}

// We try it all here, if CPUID doesn't work we go to RTC and if that doesn't work
// we're fucked
static void calibrate_cpu_timing(void) {
    if (cpu_cycles_per_10ms != 0) // We already calibrated (somehow) 
        return;  
    
    kprintf("=== CPU Timing Calibration ===\n");
    
    // Try CPUID first
    uint64_t cpu_freq_mhz = get_cpu_frequency_mhz();
    if (cpu_freq_mhz > 0) {
        cpu_cycles_per_10ms = cpu_freq_mhz * 10000;  // MHz * 10ms * 1000
        kprintf("CPUID calibration: %lu MHz CPU = %lu cycles per 10ms\n", 
                cpu_freq_mhz, cpu_cycles_per_10ms);
        return;
    }
    
    KWARN("CPUID frequency detection failed, trying RTC...\n");
    
    // Try RTC calibration
    calibrate_with_rtc();
    if (cpu_cycles_per_10ms > 0) {
        // Sanity check: should be between 1M and 100M for reasonable CPUs
        if (cpu_cycles_per_10ms < 1000000 || cpu_cycles_per_10ms > 100000000) {
            kprintf("RTC calibration seems wrong: %lu cycles\n", cpu_cycles_per_10ms);
            cpu_cycles_per_10ms = 0;
        } else {
            KSUCCESS("RTC calibration successful\n");
            return;
        }
    }
    
    KWARN("RTC calibration failed, using known frequency estimate...\n");
    calibrate_with_known_frequencies();
}

void cpu_wait_10ms(void) {
    if (cpu_cycles_per_10ms == 0) {
        calibrate_cpu_timing();
    }
    
    uint64_t start = get_cpu_cycles();
    uint64_t target = start + cpu_cycles_per_10ms;
    
    while (get_cpu_cycles() < target) {
        asm volatile("pause");
    }
}


static void test_timing_methods(void) {
    kprintf("=== Comprehensive Timing Test ===\n");
    
    calibrate_cpu_timing();
    
    kprintf("\nTesting calibration 5 times:\n");
    
        
    uint64_t total_cycles = 0;
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    
    for (int i = 0; i < 5; i++) {
        uint64_t start = get_cpu_cycles();
        cpu_wait_10ms();
        uint64_t end = get_cpu_cycles();
        
        uint64_t elapsed = end - start;
        total_cycles += elapsed;
        
        if (elapsed < min_cycles) min_cycles = elapsed;
        if (elapsed > max_cycles) max_cycles = elapsed;
        
        kprintf("  Test %d: %lu cycles\n", i + 1, elapsed);
    }
    
    uint64_t avg_cycles = total_cycles / 5;
    uint64_t variance = max_cycles - min_cycles;
    
    kprintf("  Average: %lu cycles\n", avg_cycles);
    kprintf("  Min: %lu, Max: %lu, Variance: %lu\n", 
            min_cycles, max_cycles, variance);
    
    // Calculate consistency percentage
    double consistency = (double)(avg_cycles - variance/2) / avg_cycles * 100.0;
    kprintf("  Consistency: %d%%\n", (int)consistency);
    
    // Additional validation: Compare CPU method against expected
    kprintf("\n=== CPU Calibration Validation ===\n");
    kprintf("Expected cycles per 10ms: %lu\n", cpu_cycles_per_10ms);
    
    // Test CPU method accuracy
    uint64_t start = get_cpu_cycles();
    cpu_wait_10ms();
    uint64_t end = get_cpu_cycles();
    uint64_t actual = end - start;
    
    double accuracy = (double)cpu_cycles_per_10ms / actual * 100.0;
    kprintf("Actual cycles used: %lu\n", actual);
    kprintf("Accuracy: %d%%\n", (int)accuracy);
    
    if (accuracy > 95.0 && accuracy < 105.0) {
        kprintf("CPU calibration appears accurate\n");
    } else {
        kprintf("CPU calibration may be inaccurate\n");
    }
}


void run_pit_tests(void) {
    kprintf("Starting comprehensive timing tests...\n");
    kprintf("CPU TSC frequency detection and calibration in progress...\n");
    
    test_timing_methods();
}

