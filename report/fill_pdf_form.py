#!/usr/bin/env python3
"""
PDF Form Filler for ESP32 WiFi Sniffer Weekly Report
Fills out the university weekly report form with project details
"""

import os
import sys
from datetime import datetime, timedelta
import json

# Try to import PDF libraries
try:
    from PyPDF2 import PdfReader, PdfWriter
    import io
    from reportlab.pdfgen import canvas
    from reportlab.lib.pagesizes import letter
    PDF_AVAILABLE = True
except ImportError:
    PDF_AVAILABLE = False
    print("PDF libraries not available. Install with: pip install PyPDF2 reportlab")

def create_project_data():
    """Create the project data for the weekly report"""
    
    # Calculate week dates (assuming 6-week project starting from a recent date)
    start_date = datetime(2024, 12, 16)  # Adjust as needed
    current_week = 1  # You can change this based on your progress
    
    week_start = start_date + timedelta(weeks=current_week-1)
    week_end = week_start + timedelta(days=6)
    
    project_data = {
        # Basic Information
        "student_name": "Almoulla Al Maawali",
        "student_id": "Your Student ID",  # Fill this in
        "course_code": "COMP 499",  # Adjust as needed
        "supervisor": "Dr. Hasan Bulut",
        "supervisor_email": "hasan.bulut@ege.edu.tr",
        "supervisor_phone": "+90 (232) 311 2596",
        
        # Week Information
        "week_number": current_week,
        "week_start_date": week_start.strftime("%d/%m/%Y"),
        "week_end_date": week_end.strftime("%d/%m/%Y"),
        "report_date": datetime.now().strftime("%d/%m/%Y"),
        
        # Project Details
        "project_title": "Wi-Fi Packet Sniffer Using ESP32 for Wireless Device Detection and Basic Network Monitoring",
        
        # Week 1 Specific Details
        "weekly_objectives": [
            "Learn Wi-Fi frame types: Beacon, Probe Request, Authentication, etc.",
            "Set up ESP32 development environment with PlatformIO",
            "Load basic Wi-Fi sniffing example and print raw frame metadata over serial"
        ],
        
        "activities_completed": [
            "✅ Set up PlatformIO development environment for ESP32",
            "✅ Created basic WiFi packet sniffer using ESP32 promiscuous mode",
            "✅ Implemented frame capture and metadata extraction (channel, MAC, RSSI, frame type)",
            "✅ Added frame type classification (Management, Control, Data frames)",
            "✅ Implemented channel hopping (switches every 2 seconds through channels 1-13)",
            "✅ Added smart filtering to reduce output spam and show only interesting frames",
            "✅ Created comprehensive README with usage instructions and legal warnings"
        ],
        
        "technical_achievements": [
            "Successfully captured WiFi management frames (beacons, probe requests/responses)",
            "Implemented real-time frame analysis with RSSI monitoring",
            "Added SSID extraction from beacon and probe request frames",
            "Created statistics tracking (interesting frames vs total frames per channel)",
            "Developed clean, well-documented code structure"
        ],
        
        "challenges_faced": [
            "Initial output was very noisy with too many frames - solved with smart filtering",
            "Understanding WiFi frame structure and MAC address positioning",
            "Managing channel switching timing to capture maximum devices"
        ],
        
        "solutions_implemented": [
            "Created is_interesting_frame() function to filter out data/control frames",
            "Added proper frame type and subtype classification",
            "Implemented channel switching with 2-second intervals for optimal coverage"
        ],
        
        "next_week_plan": [
            "Implement device tracking and MAC address registry",
            "Add signal strength grouping (strong, medium, weak)",
            "Develop anomaly detection for probe floods and deauth attacks",
            "Create device activity statistics and reporting"
        ],
        
        "hours_spent": "15-20 hours",
        "code_lines_written": "~300 lines of C++ code",
        "files_created": "main.cpp, platformio.ini, README.md",
        
        # Technical Specifications
        "hardware_used": "ESP32 Wrover Dev Board",
        "software_used": "PlatformIO, Arduino Framework, Serial Monitor",
        "libraries_used": "WiFi, esp_wifi, esp_event, esp_log, nvs_flash",
        
        # Deliverables Status
        "deliverables_completed": [
            "✅ Working development setup with PlatformIO",
            "✅ Captured frames with channel, MAC, RSSI, frame type over UART",
            "✅ Frame filtering and classification system",
            "✅ Channel hopping implementation",
            "✅ GitHub repository with complete documentation"
        ],
        
        "repository_url": "https://github.com/oulla898/esp-sniffer",
        "demo_available": "Yes - Real-time WiFi packet capture via serial monitor"
    }
    
    return project_data

def generate_text_report(data):
    """Generate a text-based report that can be copied into PDF forms"""
    
    report = f"""
WEEKLY REPORT FORM - ESP32 WiFi Sniffer Project
===============================================

STUDENT INFORMATION:
-------------------
Name: {data['student_name']}
Student ID: {data['student_id']}
Course Code: {data['course_code']}

SUPERVISOR INFORMATION:
---------------------
Name: {data['supervisor']}
Email: {data['supervisor_email']}
Phone: {data['supervisor_phone']}

WEEK INFORMATION:
----------------
Week Number: {data['week_number']}
Week Period: {data['week_start_date']} to {data['week_end_date']}
Report Date: {data['report_date']}

PROJECT DETAILS:
---------------
Project Title: {data['project_title']}

WEEKLY OBJECTIVES:
-----------------
"""
    
    for i, objective in enumerate(data['weekly_objectives'], 1):
        report += f"{i}. {objective}\n"
    
    report += f"""
ACTIVITIES COMPLETED:
--------------------
"""
    
    for activity in data['activities_completed']:
        report += f"• {activity}\n"
    
    report += f"""
TECHNICAL ACHIEVEMENTS:
----------------------
"""
    
    for achievement in data['technical_achievements']:
        report += f"• {achievement}\n"
    
    report += f"""
CHALLENGES FACED:
----------------
"""
    
    for challenge in data['challenges_faced']:
        report += f"• {challenge}\n"
    
    report += f"""
SOLUTIONS IMPLEMENTED:
---------------------
"""
    
    for solution in data['solutions_implemented']:
        report += f"• {solution}\n"
    
    report += f"""
NEXT WEEK PLAN:
--------------
"""
    
    for plan in data['next_week_plan']:
        report += f"• {plan}\n"
    
    report += f"""
PROJECT METRICS:
---------------
Hours Spent: {data['hours_spent']}
Code Lines Written: {data['code_lines_written']}
Files Created: {data['files_created']}

TECHNICAL SPECIFICATIONS:
------------------------
Hardware Used: {data['hardware_used']}
Software Used: {data['software_used']}
Libraries Used: {data['libraries_used']}

DELIVERABLES STATUS:
-------------------
"""
    
    for deliverable in data['deliverables_completed']:
        report += f"• {deliverable}\n"
    
    report += f"""
ADDITIONAL INFORMATION:
----------------------
Repository URL: {data['repository_url']}
Demo Available: {data['demo_available']}

SIGNATURE: _____________________
DATE: {data['report_date']}
"""
    
    return report

def save_text_report(data):
    """Save the report as a text file"""
    report_text = generate_text_report(data)
    
    filename = f"Weekly_Report_Week_{data['week_number']}_{datetime.now().strftime('%Y%m%d')}.txt"
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(report_text)
    
    print(f"Text report saved as: {filename}")
    return filename

def main():
    """Main function to generate the weekly report"""
    
    print("ESP32 WiFi Sniffer - Weekly Report Generator")
    print("=" * 50)
    
    # Create project data
    data = create_project_data()
    
    # Generate and save text report
    filename = save_text_report(data)
    
    print(f"\nReport generated successfully!")
    print(f"File: {filename}")
    print(f"Repository: {data['repository_url']}")
    
    print(f"\nYou can:")
    print(f"1. Copy the content from {filename} into your PDF form")
    print(f"2. Use online PDF fillers with the text content")
    print(f"3. Print the text file and attach to your submission")
    
    if not PDF_AVAILABLE:
        print(f"\nTo enable PDF generation, install: pip install PyPDF2 reportlab")
    
    print(f"\nCurrent Progress: Week {data['week_number']} of 6")
    print(f"Ready for submission!")

if __name__ == "__main__":
    main() 