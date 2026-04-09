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
FIRMWARE="bin/firmware_boot.bin"

# Build if needed
build_if_needed() {
    if [ ! -f "$EMULATOR" ]; then
        echo -e "${YELLOW}Building emulator...${NC}"
        make
    fi
    
    # Build firmware if needed
    if [ ! -f "$FIRMWARE" ]; then
        echo -e "${YELLOW}Building firmware...${NC}"
        make tests
    fi
    
    # Check if any user binaries exist
    if [ -z "$(ls bin/*.bin 2>/dev/null | grep -v firmware_ | head -1)" ]; then
        echo -e "${YELLOW}Building user programs...${NC}"
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
    local names=()
    
    # First show firmware options (if any)
    for bin in bin/firmware_*.bin; do
        if [ -f "$bin" ]; then
            name=$(basename "$bin" .bin)
            options[idx]="$bin"
            names[idx]="$name"
            echo -e "  ${GREEN}$idx.${NC} Run firmware: ${name#firmware_}"
            ((idx++))
        fi
    done
    
    # Then show user programs (skip firmware)
    for bin in bin/*.bin; do
        if [ -f "$bin" ] && [[ ! "$bin" =~ firmware_ ]]; then
            name=$(basename "$bin" .bin)
            options[idx]="$bin"
            names[idx]="$name"
            echo -e "  ${GREEN}$idx.${NC} $name"
            ((idx++))
        fi
    done
    
    echo ""
    echo -e "  ${YELLOW}a.${NC} Run all user programs (with firmware)"
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
                    run_program "${options[$choice]}" "${names[$choice]}"
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
    local name=$2
    
    # Check if it's a firmware or user program
    if [[ "$bin" =~ firmware_ ]]; then
        # For firmware, we need a user program to run with it
        echo ""
        echo -e "${YELLOW}Running firmware '$name' requires a user program.${NC}"
        echo -e "${YELLOW}Please select a user program to run with the firmware:${NC}"
        echo ""
        
        # List available user programs
        local user_idx=1
        local user_options=()
        local user_names=()
        
        for user_bin in bin/*.bin; do
            if [ -f "$user_bin" ] && [[ ! "$user_bin" =~ firmware_ ]]; then
                user_name=$(basename "$user_bin" .bin)
                user_options[user_idx]="$user_bin"
                user_names[user_idx]="$user_name"
                echo -e "  ${GREEN}$user_idx.${NC} $user_name"
                ((user_idx++))
            fi
        done
        
        if [ $user_idx -eq 1 ]; then
            echo -e "${RED}No user programs found! Please build some first.${NC}"
            read -p "Press Enter to continue..."
            return
        fi
        
        echo ""
        read -p "Select user program (1-$((user_idx-1))): " user_choice
        
        if [ $user_choice -ge 1 ] && [ $user_choice -lt $user_idx ]; then
            echo ""
            echo -e "${BLUE}========================================${NC}"
            echo -e "${BLUE}Running: $name with ${user_names[$user_choice]}${NC}"
            echo -e "${BLUE}========================================${NC}"
            echo -e "${YELLOW}Firmware: $bin${NC}"
            echo -e "${YELLOW}User program: ${user_options[$user_choice]}${NC}"
            echo -e "${YELLOW}Memory layout: Firmware at 0x0000, User at 0x2000${NC}"
            echo ""
            
            "$EMULATOR" "${user_options[$user_choice]}"
            
            echo -e "${BLUE}========================================${NC}"
        else
            echo -e "${RED}Invalid selection!${NC}"
        fi
    else
        # Regular user program - run with default firmware
        echo ""
        echo -e "${BLUE}========================================${NC}"
        echo -e "${BLUE}Running: $name${NC}"
        echo -e "${BLUE}========================================${NC}"
        echo -e "${YELLOW}Firmware: $FIRMWARE${NC}"
        echo -e "${YELLOW}User program: $bin${NC}"
        echo -e "${YELLOW}Memory layout: Firmware at 0x0000, User at 0x2000${NC}"
        echo ""
        
        "$EMULATOR" "$bin"
        
        echo -e "${BLUE}========================================${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

# Run all user programs
run_all() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Running all user programs${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Check if firmware exists
    if [ ! -f "$FIRMWARE" ]; then
        echo -e "${RED}Firmware not found! Please build it first.${NC}"
        read -p "Press Enter to continue..."
        return
    fi
    
    local count=0
    for bin in bin/*.bin; do
        if [ -f "$bin" ] && [[ ! "$bin" =~ firmware_ ]]; then
            name=$(basename "$bin" .bin)
            echo ""
            echo -e "${YELLOW}Running: $name${NC}"
            echo -e "${YELLOW}----------------------------------------${NC}"
            "$EMULATOR" "$bin" || true
            echo -e "${YELLOW}----------------------------------------${NC}"
            ((count++))
        fi
    done
    
    if [ $count -eq 0 ]; then
        echo -e "${RED}No user programs found!${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

# List programs
list_programs() {
    echo ""
    echo -e "${BLUE}Firmware:${NC}"
    echo -e "${BLUE}---------${NC}"
    for bin in bin/firmware_*.bin; do
        if [ -f "$bin" ]; then
            name=$(basename "$bin" .bin)
            size=$(stat -c%s "$bin" 2>/dev/null || stat -f%z "$bin" 2>/dev/null)
            echo -e "  ${GREEN}${name#firmware_}${NC} (${size} bytes) - loads at 0x0000"
        fi
    done
    
    echo ""
    echo -e "${BLUE}User Programs (run at 0x2000):${NC}"
    echo -e "${BLUE}-------------------------------${NC}"
    for bin in bin/*.bin; do
        if [ -f "$bin" ] && [[ ! "$bin" =~ firmware_ ]]; then
            name=$(basename "$bin" .bin)
            size=$(stat -c%s "$bin" 2>/dev/null || stat -f%z "$bin" 2>/dev/null)
            echo -e "  ${GREEN}$name${NC} (${size} bytes)"
        fi
    done
    
    if [ -z "$(ls bin/*.bin 2>/dev/null | grep -v firmware_ | head -1)" ]; then
        echo -e "${YELLOW}  No user programs found!${NC}"
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