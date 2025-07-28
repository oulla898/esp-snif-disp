from PyPDF2 import PdfReader, PdfWriter
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import letter
from reportlab.lib.units import mm
import io

# Report data
student_name = "Almoulla Al Maawali"
student_id = "Your Student ID"
course_code = "COMP 499"
supervisor = "Dr. Hasan Bulut"
supervisor_email = "hasan.bulut@ege.edu.tr"
supervisor_phone = "+90 (232) 311 2596"
week_number = "1"
week_period = "16/12/2024 to 22/12/2024"
report_date = "10/07/2025"
project_title = "Wi-Fi Packet Sniffer Using ESP32 for Wireless Device Detection and Basic Network Monitoring"
objectives = "1. Learn Wi-Fi frame types\n2. Set up ESP32 dev env\n3. Load sniffer example"
activities = "Set up PlatformIO, Created sniffer, Frame capture, Filtering, Channel hopping, README"
achievements = "Captured management frames, Real-time analysis, SSID extraction, Stats tracking"
challenges = "Noisy output, Frame structure, Channel timing"
solutions = "Filtering, Classification, Channel switching"
next_week = "Device tracking, Signal grouping, Anomaly detection, Stats reporting"
metrics = "15-20 hours, ~300 lines, main.cpp, platformio.ini, README.md"
hardware = "ESP32 Wrover Dev Board"
software = "PlatformIO, Arduino Framework, Serial Monitor"
libraries = "WiFi, esp_wifi, esp_event, esp_log, nvs_flash"
deliverables = "Dev setup, Frame capture, Filtering, Channel hopping, GitHub repo"
repo_url = "https://github.com/oulla898/esp-sniffer"
demo = "Yes - Real-time WiFi packet capture via serial monitor"

# Create a PDF overlay
packet = io.BytesIO()
can = canvas.Canvas(packet, pagesize=letter)

# Example coordinates (in points). Adjust as needed for your form!
can.setFont("Helvetica", 10)
can.drawString(30*mm, 260*mm, student_name)
can.drawString(100*mm, 260*mm, student_id)
can.drawString(30*mm, 250*mm, course_code)
can.drawString(100*mm, 250*mm, supervisor)
can.drawString(100*mm, 245*mm, supervisor_email)
can.drawString(100*mm, 240*mm, supervisor_phone)
can.drawString(30*mm, 240*mm, week_number)
can.drawString(50*mm, 240*mm, week_period)
can.drawString(100*mm, 235*mm, report_date)
can.drawString(30*mm, 230*mm, project_title)
can.drawString(30*mm, 220*mm, objectives)
can.drawString(30*mm, 210*mm, activities)
can.drawString(30*mm, 200*mm, achievements)
can.drawString(30*mm, 190*mm, challenges)
can.drawString(30*mm, 180*mm, solutions)
can.drawString(30*mm, 170*mm, next_week)
can.drawString(30*mm, 160*mm, metrics)
can.drawString(30*mm, 150*mm, hardware)
can.drawString(30*mm, 145*mm, software)
can.drawString(30*mm, 140*mm, libraries)
can.drawString(30*mm, 135*mm, deliverables)
can.drawString(30*mm, 130*mm, repo_url)
can.drawString(30*mm, 125*mm, demo)

can.save()
packet.seek(0)

# Read your original PDF
existing_pdf = PdfReader(open("Weekly Report Form (1).pdf", "rb"))
output = PdfWriter()

# Merge overlay onto the first page
overlay_pdf = PdfReader(packet)
page = existing_pdf.pages[0]
page.merge_page(overlay_pdf.pages[0])
output.add_page(page)

# Add any additional pages if needed
for i in range(1, len(existing_pdf.pages)):
    output.add_page(existing_pdf.pages[i])

# Save the result
with open("Weekly_Report_Filled.pdf", "wb") as f:
    output.write(f)

print("Filled PDF saved as Weekly_Report_Filled.pdf") 