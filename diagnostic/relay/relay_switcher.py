#!/usr/bin/env python3
"""
Relay Switcher - ESP32 Fish Tank Automation Diagnostic Tool
Cycles through each appliance one at a time to test relay functionality.
Each appliance turns ON for 5 seconds while others remain OFF.
"""

import sys
import time
from datetime import datetime

import requests

# Configuration
ESP32_IP = "192.168.29.75"  # ⚠️ CHANGE THIS to your ESP32's IP address (check /status endpoint or Serial Monitor)
API_KEY = "Automate@123"
BASE_URL = f"http://{ESP32_IP}"
SWITCH_INTERVAL = 5  # seconds

# List of appliances in the order they'll be tested
APPLIANCES = ["Filter", "CO2", "Light", "Heater", "HangOnFilter", "WaveMaker"]

# ANSI color codes for terminal output
class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_header():
    """Print script header"""
    print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*60}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}   ESP32 Relay Switcher - Diagnostic Tool{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{'='*60}{Colors.RESET}\n")
    print(f"{Colors.YELLOW}Target ESP32:{Colors.RESET} {ESP32_IP}")
    print(f"{Colors.YELLOW}Switch Interval:{Colors.RESET} {SWITCH_INTERVAL} seconds")
    print(f"{Colors.YELLOW}Appliances:{Colors.RESET} {', '.join(APPLIANCES)}\n")

def turn_all_off():
    """Turn all appliances OFF"""
    payload = {
        "appliances": [
            {"name": appliance, "action": "OFF", "timeout_minutes": 5}
            for appliance in APPLIANCES
        ]
    }
    return send_control_command(payload, "All OFF")

def turn_on_single(appliance_name):
    """Turn on a single appliance and turn off all others"""
    payload = {
        "appliances": [
            {"name": appliance, "action": "ON" if appliance == appliance_name else "OFF", "timeout_minutes": 5}
            for appliance in APPLIANCES
        ]
    }
    return send_control_command(payload, f"{appliance_name} ON")

def send_control_command(payload, description):
    """Send control command to ESP32"""
    headers = {
        "X-API-Key": API_KEY,
        "Content-Type": "application/json"
    }
    
    try:
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{Colors.BLUE}{timestamp}{Colors.RESET}] {description}...", end=" ")
        
        response = requests.post(
            f"{BASE_URL}/control",
            json=payload,
            headers=headers,
            timeout=10  # Increased to 10 seconds for slow networks
        )
        
        if response.status_code == 200:
            print(f"{Colors.GREEN}✓ Success{Colors.RESET}")
            return True
        else:
            print(f"{Colors.RED}✗ Failed (Status: {response.status_code}){Colors.RESET}")
            print(f"  Response: {response.text}")
            return False
            
    except requests.exceptions.ConnectionError:
        print(f"{Colors.RED}✗ Connection Error{Colors.RESET}")
        print(f"  {Colors.YELLOW}Cannot connect to {ESP32_IP}{Colors.RESET}")
        print(f"  {Colors.YELLOW}Check ESP32 IP address and network connection{Colors.RESET}")
        return False
    except requests.exceptions.Timeout:
        print(f"{Colors.RED}✗ Timeout{Colors.RESET}")
        return False
    except Exception as e:
        print(f"{Colors.RED}✗ Error: {str(e)}{Colors.RESET}")
        return False

def get_status():
    """Get current system status"""
    headers = {"X-API-Key": API_KEY}
    
    try:
        response = requests.get(
            f"{BASE_URL}/status",
            headers=headers,
            timeout=10  # Increased to 10 seconds for slow networks
        )
        
        if response.status_code == 200:
            data = response.json()
            return data
        return None
    except:
        return None

def display_status(status_data):
    """Display current appliance states"""
    if not status_data or "appliances" not in status_data:
        return
    
    # Display temperature
    temp = status_data.get("temperature_celsius", "N/A")
    if temp == -127.0:
        temp_str = f"{Colors.RED}{temp}°C (SENSOR ERROR!){Colors.RESET}"
    elif isinstance(temp, (int, float)):
        temp_str = f"{Colors.CYAN}{temp:.1f}°C{Colors.RESET}"
    else:
        temp_str = f"{temp}°C"
    
    print(f"  Temperature: {temp_str}")
    
    appliances = status_data["appliances"]
    states = []
    
    for name in APPLIANCES:
        if name in appliances:
            state = appliances[name]["state"]
            mode = appliances[name].get("mode", "UNKNOWN")
            
            if state == "ON":
                # Highlight if heater is temperature-controlled
                if name == "Heater" and mode == "TEMP_CONTROLLED":
                    states.append(f"{Colors.GREEN}{name}: ON{Colors.RESET} {Colors.YELLOW}(TEMP)⚠{Colors.RESET}")
                else:
                    states.append(f"{Colors.GREEN}{name}: ON{Colors.RESET}")
            else:
                states.append(f"{name}: OFF")
        else:
            states.append(f"{name}: ?")
    
    print(f"  Current: {' | '.join(states)}")

def run_test_cycle():
    """Run one complete test cycle through all appliances"""
    print(f"\n{Colors.BOLD}Starting test cycle...{Colors.RESET}\n")
    
    # First, turn everything off
    if not turn_all_off():
        print(f"\n{Colors.RED}Failed to initialize. Check connection and try again.{Colors.RESET}\n")
        return False
    
    time.sleep(1)  # Wait 1 second before reading status to allow ESP32 to update
    status = get_status()
    if status:
        display_status(status)
    
    time.sleep(2)
    
    # Cycle through each appliance
    for i, appliance in enumerate(APPLIANCES, 1):
        print(f"\n{Colors.BOLD}[{i}/{len(APPLIANCES)}]{Colors.RESET} Testing {Colors.CYAN}{appliance}{Colors.RESET}")
        
        if not turn_on_single(appliance):
            print(f"{Colors.YELLOW}  Skipping to next appliance...{Colors.RESET}")
            time.sleep(1)
            continue
        
        time.sleep(1)  # Wait 1 second before reading status to allow ESP32 to update
        status = get_status()
        if status:
            display_status(status)
        
        print(f"  {Colors.YELLOW}Waiting {SWITCH_INTERVAL} seconds...{Colors.RESET}")
        time.sleep(SWITCH_INTERVAL)
    
    # Turn everything off at the end
    print(f"\n{Colors.BOLD}Finishing test cycle...{Colors.RESET}\n")
    turn_all_off()
    time.sleep(1)  # Wait 1 second before reading status to allow ESP32 to update
    status = get_status()
    if status:
        display_status(status)
    
    return True

def continuous_mode():
    """Run continuous test cycles"""
    print(f"\n{Colors.BOLD}{Colors.YELLOW}Running in CONTINUOUS mode{Colors.RESET}")
    print(f"{Colors.YELLOW}Press Ctrl+C to stop{Colors.RESET}\n")
    
    cycle_count = 0
    try:
        while True:
            cycle_count += 1
            print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*60}{Colors.RESET}")
            print(f"{Colors.BOLD}{Colors.CYAN}  Cycle #{cycle_count}{Colors.RESET}")
            print(f"{Colors.BOLD}{Colors.CYAN}{'='*60}{Colors.RESET}")
            
            if not run_test_cycle():
                print(f"\n{Colors.RED}Stopping due to errors.{Colors.RESET}")
                break
            
            print(f"\n{Colors.GREEN}✓ Cycle #{cycle_count} complete{Colors.RESET}")
            print(f"{Colors.YELLOW}Waiting 5 seconds before next cycle...{Colors.RESET}")
            time.sleep(5)
            
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}Interrupted by user{Colors.RESET}")
        print(f"Turning all appliances OFF...")
        turn_all_off()
        print(f"\n{Colors.GREEN}Cleanup complete. Total cycles: {cycle_count}{Colors.RESET}\n")

def single_appliance_mode(appliance_name):
    """Test a single appliance ON/OFF repeatedly"""
    print(f"\n{Colors.BOLD}{Colors.YELLOW}Running SINGLE APPLIANCE mode: {appliance_name}{Colors.RESET}")
    print(f"{Colors.YELLOW}Press Ctrl+C to stop{Colors.RESET}\n")
    
    cycle_count = 0
    try:
        while True:
            cycle_count += 1
            print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*60}{Colors.RESET}")
            print(f"{Colors.BOLD}{Colors.CYAN}  {appliance_name} - Cycle #{cycle_count}{Colors.RESET}")
            print(f"{Colors.BOLD}{Colors.CYAN}{'='*60}{Colors.RESET}\n")
            
            # Turn ON the specified appliance, turn OFF all others
            print(f"{Colors.BOLD}Turning {appliance_name} ON (all others OFF){Colors.RESET}")
            payload = {
                "appliances": [
                    {"name": app, "action": "ON" if app == appliance_name else "OFF", "timeout_minutes": 5}
                    for app in APPLIANCES
                ]
            }
            if not send_control_command(payload, f"{appliance_name} ON"):
                print(f"\n{Colors.RED}Failed to turn on {appliance_name}. Stopping.{Colors.RESET}\n")
                break
            
            time.sleep(1)  # Wait 1 second before reading status
            status = get_status()
            if status:
                display_status(status)
            
            print(f"\n{Colors.YELLOW}Waiting {SWITCH_INTERVAL} seconds...{Colors.RESET}")
            time.sleep(SWITCH_INTERVAL)
            
            # Turn OFF the appliance (and keep all others OFF)
            print(f"\n{Colors.BOLD}Turning {appliance_name} OFF{Colors.RESET}")
            payload = {
                "appliances": [
                    {"name": app, "action": "OFF", "timeout_minutes": 5}
                    for app in APPLIANCES
                ]
            }
            if not send_control_command(payload, f"{appliance_name} OFF"):
                print(f"\n{Colors.RED}Failed to turn off {appliance_name}. Stopping.{Colors.RESET}\n")
                break
            
            time.sleep(1)  # Wait 1 second before reading status
            status = get_status()
            if status:
                display_status(status)
            
            print(f"\n{Colors.GREEN}✓ Cycle #{cycle_count} complete{Colors.RESET}")
            print(f"{Colors.YELLOW}Waiting 3 seconds before next cycle...{Colors.RESET}")
            time.sleep(3)
            
    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}Interrupted by user{Colors.RESET}")
        print(f"Turning all appliances OFF...")
        turn_all_off()
        print(f"\n{Colors.GREEN}Cleanup complete. Total cycles: {cycle_count}{Colors.RESET}\n")

def main():
    """Main function"""
    print_header()
    
    # Check for single appliance mode
    if len(sys.argv) > 2 and sys.argv[1] in ["-a", "--appliance"]:
        appliance_name = sys.argv[2]
        # Case-insensitive matching
        matched_appliance = None
        for app in APPLIANCES:
            if app.lower() == appliance_name.lower():
                matched_appliance = app
                break
        
        if matched_appliance is None:
            print(f"{Colors.RED}Error: '{appliance_name}' is not a valid appliance.{Colors.RESET}")
            print(f"{Colors.YELLOW}Valid appliances: {', '.join(APPLIANCES)}{Colors.RESET}\n")
            sys.exit(1)
        single_appliance_mode(matched_appliance)
    # Check if user wants continuous mode
    elif len(sys.argv) > 1 and sys.argv[1] in ["-c", "--continuous"]:
        continuous_mode()
    else:
        # Single cycle mode
        print(f"{Colors.BOLD}Running SINGLE test cycle{Colors.RESET}")
        print(f"{Colors.YELLOW}(Use --continuous or -c flag for continuous mode){Colors.RESET}\n")
        
        if run_test_cycle():
            print(f"\n{Colors.GREEN}{Colors.BOLD}✓ Test cycle complete!{Colors.RESET}\n")
            print(f"{Colors.YELLOW}Tips:{Colors.RESET}")
            print(f"  • Check if you heard relay clicks for each appliance")
            print(f"  • Verify LEDs on relay board switched correctly")
            print(f"  • Run with --continuous flag for repeated testing")
            print()
        else:
            print(f"\n{Colors.RED}Test failed. Please check the errors above.{Colors.RESET}\n")
            sys.exit(1)

if __name__ == "__main__":
    main()
