#!/bin/bash

echo "Running sparse static analysis on all C files..."

find . -name "*.c" -type f | while read -r file; do
    echo "Analyzing: $file"
    sparse \
        -D__is_kernel \
        -Ikernel/include \
        -Ikernel/tests \
        -ffreestanding \
        -mcmodel=large \
        -mno-red-zone \
        -mno-mmx \
        -mno-sse \
        -mno-sse2 \
        -fno-stack-protector \
        -fno-exceptions \
        -fPIC \
        -Wall \
        -Waddress-space \
        -Wbitwise \
        -Wcast-to-as \
        -Wcast-truncate \
        -Wcontext \
        -Wdecl \
        -Wdefault-bitfield-sign \
        -Wdesignated-init \
        -Wdo-while \
        -Wenum-mismatch \
        -Winit-cstring \
        -Wmemcpy-max-count \
        -Wnon-pointer-null \
        -Wold-initializer \
        -Wold-style-definition \
        -Wone-bit-signed-bitfield \
        -Wparen-string \
        -Wptr-subtraction-blows \
        -Wreturn-void \
        -Wshadow \
        -Wsizeof-bool \
        -Wsparse-error \
        -Wtautological-compare \
        -Wtransparent-union \
        -Wtypesign \
        -Wundef \
        -Wuninitialized \
        -Wunknown-attribute \
        -Wvla \
        "$file" || true
    echo "---"
done

echo "Sparse analysis complete!"
