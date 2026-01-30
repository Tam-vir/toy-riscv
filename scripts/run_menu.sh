#!/bin/bash
# Interactive runner for RISC-V emulator

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

EMULATOR="bin/rvemu"

# Build if needed
build_if_needed() {
    if [ ! -f "$EMULATOR" ]; then
        echo -e "${YELLOW}Building emulator...${NC}"
        make
    fi
    
    # Check if any binaries exist
    if [ -z "$(ls bin/*.bin 2>/dev/null)" ]; then
        echo -e "${YELLOW}Building test programs...${NC}"
        make tests
    fi
}

# Show menu
show_menu() {
    clear
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}    RISC-V Emulator - Main Menu${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    local idx=1
    local options=()
    
    # Find all binaries
    for bin in bin/*.bin; do
        if [ -f "$bin" ]; then
            name=$(basename "$bin" .bin)
            options[idx]="$bin"
            echo -e "  ${GREEN}$idx.${NC} Run $name"
            ((idx++))
        fi
    done
    
    echo ""
    echo -e "  ${YELLOW}a.${NC} Run all programs"
    echo -e "  ${YELLOW}b.${NC} Rebuild everything"
    echo -e "  ${YELLOW}l.${NC} List available programs"
    echo -e "  ${YELLOW}c.${NC} Clean build files"
    echo -e "  ${YELLOW}q.${NC} Quit"
    echo ""
    
    total_options=$((idx-1))
    
    while true; do
        read -p "Select option: " choice
        
        case $choice in
            [1-9]*)
                if [ $choice -ge 1 ] && [ $choice -le $total_options ]; then
                    run_program "${options[$choice]}"
                    return 1
                else
                    echo -e "${RED}Invalid selection!${NC}"
                fi
                ;;
            a|A)
                run_all
                return 1
                ;;
            b|B)
                rebuild
                return 0
                ;;
            l|L)
                list_programs
                return 1
                ;;
            c|C)
                make clean
                echo -e "${GREEN}Cleaned!${NC}"
                sleep 1
                return 0
                ;;
            q|Q)
                echo -e "${BLUE}Goodbye!${NC}"
                exit 0
                ;;
            *)
                echo -e "${RED}Invalid selection!${NC}"
                ;;
        esac
    done
}

# Run a program
run_program() {
    local bin=$1
    local name=$(basename "$bin" .bin)
    
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Running: $name${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    "$EMULATOR" "$bin"
    
    echo -e "${BLUE}========================================${NC}"
    echo ""
    read -p "Press Enter to continue..."
}

# Run all programs
run_all() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Running all programs${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    for bin in bin/*.bin; do
        if [ -f "$bin" ]; then
            name=$(basename "$bin" .bin)
            echo ""
            echo -e "${YELLOW}Running: $name${NC}"
            echo -e "${YELLOW}----------------------------------------${NC}"
            "$EMULATOR" "$bin" || true
            echo -e "${YELLOW}----------------------------------------${NC}"
        fi
    done
    
    echo ""
    read -p "Press Enter to continue..."
}

# List programs
list_programs() {
    echo ""
    echo -e "${BLUE}Available programs:${NC}"
    echo -e "${BLUE}-------------------${NC}"
    
    for bin in bin/*.bin; do
        if [ -f "$bin" ]; then
            name=$(basename "$bin" .bin)
            size=$(stat -c%s "$bin" 2>/dev/null || stat -f%z "$bin")
            echo -e "  ${GREEN}$name${NC} (${size} bytes)"
        fi
    done
    
    if [ -z "$(ls bin/*.bin 2>/dev/null)" ]; then
        echo -e "${YELLOW}No programs found!${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

# Rebuild everything
rebuild() {
    echo -e "${YELLOW}Rebuilding everything...${NC}"
    make clean
    make
    make tests
    echo -e "${GREEN}Rebuild complete!${NC}"
    sleep 1
}

# Main loop
main() {
    while true; do
        build_if_needed
        if show_menu; then
            continue
        fi
    done
}

# Run main
main