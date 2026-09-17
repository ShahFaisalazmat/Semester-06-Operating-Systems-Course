#!/bin/bash

# ==============================================
# server_monitor.sh - Server Resource Monitor
# Author: Shah Faisal
# Roll: 23I-0058
# Description: Monitors disk, CPU, RAM and manages log rotation
# ==============================================

# Configuration
THRESHOLD_DISK=50          
THRESHOLD_CPU=50           
THRESHOLD_MEM=30     
LOG_FILE="/var/log/server_monitor.log"
WATCH_LOG="/var/log/application.log"  
MAX_LOG_SIZE=25            
TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")

# ==============================================
# Initialize logging
# ==============================================
setup_logging() {
    # Create log directory if it doesn't exist
    sudo mkdir -p /var/log 2>/dev/null
    
    # Create log file if it doesn't exist
    if [ ! -f "$LOG_FILE" ]; then
        sudo touch "$LOG_FILE"
        sudo chmod 644 "$LOG_FILE"
    fi
    
    echo "$TIMESTAMP - Server Monitor Started" | sudo tee -a "$LOG_FILE"
}

# ==============================================
# Logging function
# ==============================================
log_message() {
    echo "$TIMESTAMP - $1" | sudo tee -a "$LOG_FILE"
}

# ==============================================
# Function 1: Disk Usage Monitoring
# ==============================================
check_disk_usage() {
    echo "=========================================="
    echo "Checking Disk Usage..."
    log_message "Checking Disk Usage..."
    
    # Get disk usage percentage for root partition
    DISK_USAGE=$(df -h / | awk 'NR==2 {print $5}' | sed 's/%//')
    DISK_USED=$(df -h / | awk 'NR==2 {print $3}')
    DISK_TOTAL=$(df -h / | awk 'NR==2 {print $2}')
    
    echo "Disk Usage: $DISK_USAGE% ($DISK_USED used of $DISK_TOTAL)"
    log_message "Disk Usage: $DISK_USAGE%"
    
    # Alert if disk usage exceeds threshold
    if [ "$DISK_USAGE" -gt "$THRESHOLD_DISK" ]; then
        echo "ALERT: Disk usage is above $THRESHOLD_DISK% (Currently: $DISK_USAGE%)"
        log_message "ALERT: High disk usage - $DISK_USAGE%"
        echo "Top 5 largest directories in /:"
        sudo du -sh /* 2>/dev/null | sort -rh | head -5
    else
        echo "Disk usage is normal (Below $THRESHOLD_DISK%)"
        log_message "Disk usage normal"
    fi
}

# ==============================================
# Function 2: CPU Usage Monitoring
# ==============================================
check_cpu_usage() {
    echo "=========================================="
    echo "Checking CPU Usage..."
    log_message "Checking CPU Usage..."
    
    # Get CPU usage (100 - idle percentage)
    CPU_IDLE=$(top -bn1 | grep "Cpu(s)" | awk '{print $8}' | cut -d',' -f1)
    CPU_USAGE=$(echo "100 - $CPU_IDLE" | bc 2>/dev/null)
    
    # Fallback if bc not available
    if [ -z "$CPU_USAGE" ]; then
        CPU_USAGE=$(top -bn1 | grep "%Cpu" | awk '{print 100 - $8}')
    fi
    
    # Get load average
    LOAD_AVG=$(uptime | awk -F'load average:' '{print $2}')
    
    echo "CPU Usage: $CPU_USAGE%"
    echo "Load Average: $LOAD_AVG"
    log_message "CPU Usage: $CPU_USAGE%"
    
    # Alert if CPU usage exceeds threshold
    if (( $(echo "$CPU_USAGE > $THRESHOLD_CPU" | bc -l 2>/dev/null) )); then
        echo "ALERT: CPU usage is above $THRESHOLD_CPU% (Currently: $CPU_USAGE%)"
        log_message "ALERT: High CPU usage - $CPU_USAGE%"
        echo "Top 5 CPU-consuming processes:"
        ps aux --sort=-%cpu | head -6
    else
        echo "CPU usage is normal (Below $THRESHOLD_CPU%)"
        log_message "CPU usage normal"
    fi
}

# ==============================================
# Function 3: Memory Usage Monitoring
# ==============================================
check_memory_usage() {
    echo "=========================================="
    echo "Checking Memory Usage..."
    log_message "Checking Memory Usage..."
    
    # Get memory information
    MEM_TOTAL=$(free -m | awk 'NR==2 {print $2}')
    MEM_USED=$(free -m | awk 'NR==2 {print $3}')
    MEM_FREE=$(free -m | awk 'NR==2 {print $4}')
    MEM_FREE_PERCENT=$((MEM_FREE * 100 / MEM_TOTAL))
    
    echo "Total RAM: ${MEM_TOTAL}MB"
    echo "Used RAM: ${MEM_USED}MB"
    echo "Free RAM: ${MEM_FREE}MB (${MEM_FREE_PERCENT}%)"
    log_message "Memory - Total: ${MEM_TOTAL}MB, Free: ${MEM_FREE_PERCENT}%"
    
    # Alert if free memory is below threshold
    if [ "$MEM_FREE_PERCENT" -lt "$THRESHOLD_MEM" ]; then
        echo "ALERT: Free memory is below $THRESHOLD_MEM% (Currently: $MEM_FREE_PERCENT%)"
        log_message "ALERT: Low memory - Only $MEM_FREE_PERCENT% free"
        
        # Show top 5 memory-consuming processes
        echo "Top 5 memory-consuming processes:"
        ps aux --sort=-%mem | head -6
    else
        echo "Memory usage is normal (Free: ${MEM_FREE_PERCENT}%)"
        log_message "Memory usage normal"
    fi
}

# ==============================================
# Function 4: Log Rotation
# ==============================================
rotate_logs() {
    echo "=========================================="
    echo "Checking Log Rotation..."
    log_message "Checking Log Rotation..."
    
    # Create sample log file if it doesn't exist
    if [ ! -f "$WATCH_LOG" ]; then
        echo "Creating sample log file: $WATCH_LOG"
        sudo touch "$WATCH_LOG"
        sudo chmod 644 "$WATCH_LOG"
        # Add some sample content
        for i in {1..100}; do
            echo "Sample log entry $i - $(date)" | sudo tee -a "$WATCH_LOG" >/dev/null
        done
    fi
    
    # Check log file size
    if [ -f "$WATCH_LOG" ]; then
        LOG_SIZE=$(sudo du -m "$WATCH_LOG" | cut -f1)
        echo "Log file: $WATCH_LOG"
        echo "Current size: ${LOG_SIZE}MB"
        log_message "Log size: ${LOG_SIZE}MB"
        
        # Rotate if log exceeds threshold
        if [ "$LOG_SIZE" -gt "$MAX_LOG_SIZE" ]; then
            echo "Log file exceeds ${MAX_LOG_SIZE}MB, rotating..."
            log_message "Rotating log file - size: ${LOG_SIZE}MB"
            
            # Create backup directory
            BACKUP_DIR="/var/log/backups"
            sudo mkdir -p "$BACKUP_DIR"
            
            # Generate timestamp for backup
            BACKUP_TIME=$(date "+%Y%m%d_%H%M%S")
            BACKUP_FILE="$BACKUP_DIR/application_$BACKUP_TIME.log"
            
            # Compress and backup
            sudo cp "$WATCH_LOG" "$BACKUP_FILE"
            sudo gzip "$BACKUP_FILE"
            echo "Log backed up to: ${BACKUP_FILE}.gz"
            
            # Clear original log
            sudo truncate -s 0 "$WATCH_LOG"
            echo "Original log cleared"
            
            log_message "Log rotation completed: ${BACKUP_FILE}.gz"
        else
            echo "Log size is normal (Below ${MAX_LOG_SIZE}MB)"
        fi
    else
        echo "Log file $WATCH_LOG not found"
        log_message "ERROR: Log file not found"
    fi
}

# ==============================================
# Function 5: Generate Summary Report
# ==============================================
generate_report() {
    REPORT_FILE="/tmp/server_report_$(date +%Y%m%d_%H%M%S).txt"
    
    {
        echo "=========================================="
        echo "SERVER MONITORING REPORT"
        echo "Generated: $(date)"
        echo "=========================================="
        echo ""
        echo "SYSTEM INFORMATION:"
        echo "-------------------"
        echo "Hostname: $(hostname)"
        echo "Kernel: $(uname -r)"
        echo "Uptime: $(uptime -p)"
        echo ""
        echo "DISK USAGE:"
        echo "-----------"
        df -h /
        echo ""
        echo "CPU INFORMATION:"
        echo "---------------"
        echo "CPU Usage: $(top -bn1 | grep '%Cpu')"
        echo "Load Avg: $(uptime | awk -F'load average:' '{print $2}')"
        echo ""
        echo "MEMORY INFORMATION:"
        echo "------------------"
        free -h
        echo ""
        echo "TOP PROCESSES:"
        echo "-------------"
        echo "By CPU:"
        ps aux --sort=-%cpu | head -5
        echo ""
        echo "By Memory:"
        ps aux --sort=-%mem | head -5
        echo ""
        echo "LOG STATUS:"
        echo "-----------"
        echo "Monitor Log: $LOG_FILE"
        echo "Watch Log: $WATCH_LOG ($(sudo du -h $WATCH_LOG | cut -f1) )"
        echo "=========================================="
    } > "$REPORT_FILE"
    
    echo "Report generated: $REPORT_FILE"
    log_message "Report generated: $REPORT_FILE"
}

# ==============================================
# Error Handling Function
# ==============================================
handle_error() {
    local error_msg="$1"
    echo "ERROR: $error_msg"
    log_message "ERROR: $error_msg"
}

# ==============================================
# Main Execution Function
# ==============================================
main() {
    echo "=========================================="
    echo "SERVER RESOURCE MONITOR"
    echo "Author: Shah Faisal (23I-0058)"
    echo "Start Time: $(date)"
    echo "=========================================="
    
    # Setup logging
    setup_logging
    
    # Check if running with proper permissions
    if [ "$EUID" -ne 0 ]; then 
        echo "Note: Some functions may require sudo access"
    fi
    
    # Execute monitoring functions with error handling
    check_disk_usage || handle_error "Disk check failed"
    check_cpu_usage || handle_error "CPU check failed"
    check_memory_usage || handle_error "Memory check failed"
    rotate_logs || handle_error "Log rotation failed"
    generate_report || handle_error "Report generation failed"
    
    echo "=========================================="
    echo "Monitoring Completed: $(date)"
    echo "Check log file: $LOG_FILE"
    echo "=========================================="
    log_message "Server monitoring completed"
}

# ==============================================
# Run main function
# ==============================================
main

# ==============================================
# Cron Setup Instructions (for automation)
# ==============================================
: '
To run this script automatically every hour, add to crontab:
0 * * * * /bin/bash /home/shah/server_monitor.sh

To run every 30 minutes:
*/30 * * * * /bin/bash /home/shah/server_monitor.sh
'