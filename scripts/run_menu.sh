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

# Global flag for interactive mode
INTERACTIVE_MODE=1

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
    if [ -z "$(ls bin/*.bin 2>/dev/null | grep -v -E '(firmware_|bootloader_)' | head -1)" ]; then
        echo -e "${YELLOW}Building user programs...${NC}"
        make tests
    fi
}

# Safe read function that handles errors gracefully
safe_read() {
    local prompt="$1"
    local var_name="$2"
    local default="${3:-}"
    
    # If not in interactive mode, use default
    if [ $INTERACTIVE_MODE -eq 0 ]; then
        if [ -n "$default" ]; then
            eval "$var_name='$default'"
        fi
        return 0
    fi
    
    # Try to read with error handling
    local input=""
    printf "%s" "$prompt" >&2
    
    # Use a temporary file descriptor to avoid read errors
    if ! read -r input < /dev/tty 2>/dev/null; then
        # Fallback: try reading from stdin
        if ! read -r input 2>/dev/null; then
            echo -e "${YELLOW}No input available. Using default: $default${NC}" >&2
            input="$default"
        fi
    fi
    
    eval "$var_name='$input'"
    return 0
}

# Simple press enter function
press_enter() {
    if [ $INTERACTIVE_MODE -eq 0 ]; then
        return 0
    fi
    
    printf "Press Enter to continue..." >&2
    # Try to read from /dev/tty first, then stdin
    read < /dev/tty 2>/dev/null || read 2>/dev/null || true
    echo >&2
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
    
    # Show user programs
    for bin in bin/*.bin; do
        if [ -f "$bin" ] && [[ ! "$bin" =~ (firmware_|bootloader_) ]]; then
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
        local choice=""
        safe_read "Select option: " choice ""
        
        case $choice in
            [1-9]*)
                if [ $choice -ge 1 ] && [ $choice -le $total_options ]; then
                    run_program "${options[$choice]}" "${names[$choice]}"
                    return 1
                else
                    echo -e "${RED}Invalid selection!${NC}"
                    sleep 1
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
            "")
                # Empty input, just loop again
                continue
                ;;
            *)
                echo -e "${RED}Invalid selection!${NC}"
                sleep 1
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
            if [ -f "$user_bin" ] && [[ ! "$user_bin" =~ (firmware_|bootloader_) ]]; then
                user_name=$(basename "$user_bin" .bin)
                user_options[user_idx]="$user_bin"
                user_names[user_idx]="$user_name"
                echo -e "  ${GREEN}$user_idx.${NC} $user_name"
                ((user_idx++))
            fi
        done
        
        if [ $user_idx -eq 1 ]; then
            echo -e "${RED}No user programs found! Please build some first.${NC}"
            press_enter
            return
        fi
        
        echo ""
        local user_choice=""
        safe_read "Select user program (1-$((user_idx-1))): " user_choice ""
        
        if [ -n "$user_choice" ] && [ $user_choice -ge 1 ] && [ $user_choice -lt $user_idx ]; then
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
            sleep 1
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
    press_enter
}

# Run all user programs
run_all() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Running all user programs${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    local count=0
    for bin in bin/*.bin; do
        if [ -f "$bin" ] && [[ ! "$bin" =~ (firmware_|bootloader_) ]]; then
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
    press_enter
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
        if [ -f "$bin" ] && [[ ! "$bin" =~ (firmware_|bootloader_) ]]; then
            name=$(basename "$bin" .bin)
            size=$(stat -c%s "$bin" 2>/dev/null || stat -f%z "$bin" 2>/dev/null)
            echo -e "  ${GREEN}$name${NC} (${size} bytes)"
        fi
    done
    
    if [ -z "$(ls bin/*.bin 2>/dev/null | grep -v -E '(firmware_|bootloader_)' | head -1)" ]; then
        echo -e "${YELLOW}  No user programs found!${NC}"
    fi
    
    echo ""
    press_enter
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

# Show help
show_help() {
    cat << EOF
${BLUE}RISC-V Emulator Runner${NC}
Usage: $0 [OPTIONS]

Options:
  --run <program>     Run a specific user program
  --run-with-fw <fw> <program>  Run a user program with specific firmware
  --list              List all available programs
  --rebuild           Rebuild everything (emulator + tests)
  --clean             Clean all build files
  --help              Show this help message
  --non-interactive   Run in non-interactive mode (use with caution)

If no options are provided, the interactive menu will be shown.

Examples:
  $0 --run hello
  $0 --run-with-fw firmware_boot hello
  $0 --list
  $0 --rebuild
EOF
}

# Parse command line arguments
parse_args() {
    # Check for non-interactive flag first
    for arg in "$@"; do
        if [ "$arg" = "--non-interactive" ]; then
            INTERACTIVE_MODE=0
            break
        fi
    done
    
    case "$1" in
        --run)
            if [ -z "$2" ]; then
                echo -e "${RED}Error: --run requires a program name${NC}"
                show_help
                exit 1
            fi
            local program="bin/$2.bin"
            if [ ! -f "$program" ]; then
                echo -e "${RED}Error: Program '$2' not found${NC}"
                echo -e "${YELLOW}Use --list to see available programs${NC}"
                exit 1
            fi
            build_if_needed
            echo -e "${BLUE}Running: $2${NC}"
            "$EMULATOR" "$program"
            exit 0
            ;;
        --run-with-fw)
            if [ -z "$2" ] || [ -z "$3" ]; then
                echo -e "${RED}Error: --run-with-fw requires firmware and program name${NC}"
                show_help
                exit 1
            fi
            local firmware="bin/$2.bin"
            local program="bin/$3.bin"
            if [ ! -f "$firmware" ]; then
                echo -e "${RED}Error: Firmware '$2' not found${NC}"
                exit 1
            fi
            if [ ! -f "$program" ]; then
                echo -e "${RED}Error: Program '$3' not found${NC}"
                exit 1
            fi
            build_if_needed
            echo -e "${BLUE}Running: $3 with firmware $2${NC}"
            "$EMULATOR" "$program"
            exit 0
            ;;
        --list)
            build_if_needed
            list_programs
            exit 0
            ;;
        --rebuild)
            rebuild
            exit 0
            ;;
        --clean)
            make clean
            echo -e "${GREEN}Cleaned!${NC}"
            exit 0
            ;;
        --help)
            show_help
            exit 0
            ;;
        --non-interactive)
            # Already handled, just continue
            return 0
            ;;
        "")
            # No arguments - run interactive menu
            return 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
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

# Script entry point
# Parse command line arguments first
parse_args "$@"

# If we get here, no arguments were provided, so run interactive menu
# Verify we're in an interactive terminal
if [ $INTERACTIVE_MODE -eq 1 ] && [ ! -t 0 ]; then
    echo -e "${YELLOW}Warning: Not running in an interactive terminal.${NC}"
    echo -e "${YELLOW}Switching to non-interactive mode. Use --help for options.${NC}"
    INTERACTIVE_MODE=0
    echo -e "${BLUE}Available programs:${NC}"
    list_programs
    exit 0
fi

# Run main interactive menu only if in interactive mode
if [ $INTERACTIVE_MODE -eq 1 ]; then
    main
else
    echo -e "${YELLOW}Non-interactive mode. Use --run to execute programs.${NC}"
    show_help
fi