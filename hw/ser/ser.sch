EESchema Schematic File Version 4
LIBS:ser-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "SER Pro Micro HW-597"
Date "2019-11-16"
Rev "1.0.0"
Comp "Daniel Starke"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L ArduinoProMicro:ArduinoProMicro IC1
U 1 1 5DCF0ED0
P 5750 3250
F 0 "IC1" H 5775 4287 60  0001 C CNN
F 1 "ArduinoProMicro" H 5775 4181 60  0000 C CNN
F 2 "footprints:ArduinoProMicro_Custom" H 5750 2350 60  0001 C CNN
F 3 "https://github.com/sparkfun/Pro_Micro" H 5850 2200 60  0001 C CNN
	1    5750 3250
	1    0    0    -1  
$EndComp
$Comp
L USB-SER:USB-SER JT1
U 1 1 5DCF1326
P 2550 2150
F 0 "JT1" H 2606 2675 50  0001 C CNN
F 1 "USB-SER" H 2606 2584 50  0000 C CNN
F 2 "footprints:PinHeader_1x06_P2.54mm_Vertical_Custom" H 2700 1850 50  0001 C CNN
F 3 "https://www.androegg.de/wp-content/uploads/2016/07/CH340G_USB_TTL_Converter.pdf" H 2700 1850 50  0001 C CNN
	1    2550 2150
	1    0    0    -1  
$EndComp
Wire Wire Line
	6550 2800 6850 2800
Wire Wire Line
	6850 2800 6850 3100
$Comp
L Isolator:ADuM1201BR U1
U 1 1 5DCF308A
P 3900 2150
F 0 "U1" H 3900 2617 50  0000 C CNN
F 1 "ADuM1201BR" H 3900 2526 50  0000 C CNN
F 2 "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm" H 3900 1750 50  0001 C CIN
F 3 "http://www.analog.com/static/imported-files/data_sheets/ADuM1200_1201.pdf" H 3900 2050 50  0001 C CNN
	1    3900 2150
	1    0    0    -1  
$EndComp
NoConn ~ 6550 2600
NoConn ~ 6550 3000
NoConn ~ 6550 3200
NoConn ~ 6550 3300
NoConn ~ 6550 3400
NoConn ~ 6550 3500
NoConn ~ 6550 3600
NoConn ~ 6550 3700
NoConn ~ 5000 3000
NoConn ~ 5000 3100
NoConn ~ 5000 3200
NoConn ~ 5000 3700
$Comp
L Device:LED D1
U 1 1 5DCF5290
P 4000 3300
F 0 "D1" H 3991 3425 50  0000 C CNN
F 1 "LED" H 3991 3425 50  0001 C CNN
F 2 "LED_SMD:LED_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 4000 3300 50  0001 C CNN
F 3 "~" H 4000 3300 50  0001 C CNN
	1    4000 3300
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D2
U 1 1 5DCF535B
P 4000 3550
F 0 "D2" H 3991 3675 50  0000 C CNN
F 1 "LED" H 3991 3675 50  0001 C CNN
F 2 "LED_SMD:LED_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 4000 3550 50  0001 C CNN
F 3 "~" H 4000 3550 50  0001 C CNN
	1    4000 3550
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D3
U 1 1 5DCF53C5
P 4000 3800
F 0 "D3" H 3991 3925 50  0000 C CNN
F 1 "LED" H 3991 3925 50  0001 C CNN
F 2 "LED_SMD:LED_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 4000 3800 50  0001 C CNN
F 3 "~" H 4000 3800 50  0001 C CNN
	1    4000 3800
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D4
U 1 1 5DCF62CA
P 4000 4050
F 0 "D4" H 3991 4175 50  0000 C CNN
F 1 "LED" H 3991 4175 50  0001 C CNN
F 2 "LED_SMD:LED_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 4000 4050 50  0001 C CNN
F 3 "~" H 4000 4050 50  0001 C CNN
	1    4000 4050
	1    0    0    -1  
$EndComp
Wire Wire Line
	5000 3400 4500 3400
Wire Wire Line
	4500 3400 4500 3550
Wire Wire Line
	5000 3500 4600 3500
Wire Wire Line
	4600 3500 4600 3800
Wire Wire Line
	4600 3800 4500 3800
Wire Wire Line
	5000 3600 4700 3600
Wire Wire Line
	4700 3600 4700 4050
Wire Wire Line
	4700 4050 4500 4050
Wire Wire Line
	3850 3300 3650 3300
Wire Wire Line
	3650 3300 3650 3100
Wire Wire Line
	3650 3100 4650 3100
Wire Wire Line
	3650 3300 3650 3550
Wire Wire Line
	3650 3550 3850 3550
Connection ~ 3650 3300
Wire Wire Line
	3650 3550 3650 3800
Wire Wire Line
	3650 3800 3850 3800
Connection ~ 3650 3550
Wire Wire Line
	3650 3800 3650 4050
Wire Wire Line
	3650 4050 3850 4050
Connection ~ 3650 3800
Wire Wire Line
	4150 3300 4200 3300
Wire Wire Line
	4650 2350 4400 2350
Wire Wire Line
	2950 2250 3150 2250
Wire Wire Line
	3150 2250 3150 2050
Wire Wire Line
	3150 2050 3400 2050
Wire Wire Line
	3400 2250 3250 2250
Wire Wire Line
	3250 2250 3250 2150
Wire Wire Line
	3250 2150 2950 2150
Wire Wire Line
	3200 1850 3200 1950
Wire Wire Line
	4400 1950 6700 1950
Wire Wire Line
	6700 2900 6550 2900
Wire Wire Line
	4400 2050 4900 2050
Wire Wire Line
	4900 2050 4900 2600
Wire Wire Line
	4900 2600 5000 2600
Wire Wire Line
	4400 2250 4800 2250
Wire Wire Line
	4800 2250 4800 2700
Wire Wire Line
	4800 2700 5000 2700
Wire Wire Line
	3650 4050 3650 4200
Connection ~ 3650 4050
Text Label 3000 1850 0    50   ~ 0
M_VDD
Text Label 3000 2350 0    50   ~ 0
M_GND
Text Label 3000 2250 0    50   ~ 0
M_RXD
Text Label 3000 2150 0    50   ~ 0
M_TXD
Text Label 4450 1950 0    50   ~ 0
S_VDD
Text Label 4450 2050 0    50   ~ 0
S_TXD
Text Label 4450 2250 0    50   ~ 0
S_RXD
Text Label 4450 2350 0    50   ~ 0
S_GND
Text Label 4800 3300 0    50   ~ 0
S_R1
Text Label 4800 3400 0    50   ~ 0
S_R2
Text Label 4800 3500 0    50   ~ 0
S_R3
Text Label 4800 3600 0    50   ~ 0
S_R4
Text Label 6600 3300 0    50   ~ 0
S_RST
Wire Wire Line
	4650 2350 4650 2800
NoConn ~ 2950 2050
NoConn ~ 2950 1950
Wire Wire Line
	6700 1950 6700 2900
Wire Wire Line
	2950 1850 3200 1850
Wire Wire Line
	2950 2350 3000 2350
$Comp
L power:GND #PWR0102
U 1 1 5DD64C7F
P 3000 2900
F 0 "#PWR0102" H 3000 2650 50  0001 C CNN
F 1 "GND" H 3005 2727 50  0000 C CNN
F 2 "" H 3000 2900 50  0001 C CNN
F 3 "" H 3000 2900 50  0001 C CNN
	1    3000 2900
	1    0    0    -1  
$EndComp
Connection ~ 3000 2350
Wire Wire Line
	3200 1950 3350 1950
Wire Wire Line
	3000 2350 3000 2900
Wire Wire Line
	6550 2700 7050 2700
Wire Wire Line
	7050 2700 7050 4200
Wire Wire Line
	7050 4200 3650 4200
Connection ~ 3650 4200
Wire Wire Line
	3650 4200 3650 4250
$Comp
L power:GND #PWR0101
U 1 1 5DD45BC1
P 3650 4250
F 0 "#PWR0101" H 3650 4000 50  0001 C CNN
F 1 "GND" H 3655 4077 50  0000 C CNN
F 2 "" H 3650 4250 50  0001 C CNN
F 3 "" H 3650 4250 50  0001 C CNN
	1    3650 4250
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5DD7436A
P 7050 2700
F 0 "#FLG0101" H 7050 2775 50  0001 C CNN
F 1 "PWR_FLAG" H 7050 2874 50  0000 C CNN
F 2 "" H 7050 2700 50  0001 C CNN
F 3 "~" H 7050 2700 50  0001 C CNN
	1    7050 2700
	1    0    0    -1  
$EndComp
Connection ~ 7050 2700
Wire Wire Line
	5000 2800 4650 2800
Connection ~ 4650 2800
Wire Wire Line
	4650 2800 4650 2900
Wire Wire Line
	5000 2900 4650 2900
Connection ~ 4650 2900
Wire Wire Line
	4650 2900 4650 3100
$Comp
L Device:R R1
U 1 1 5DCFC328
P 4350 3300
F 0 "R1" V 4250 3200 50  0000 C CNN
F 1 "500" V 4250 3350 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad1.05x0.95mm_HandSolder" V 4280 3300 50  0001 C CNN
F 3 "~" H 4350 3300 50  0001 C CNN
	1    4350 3300
	0    1    1    0   
$EndComp
Wire Wire Line
	4500 3300 5000 3300
$Comp
L Device:R R2
U 1 1 5DCFC3C3
P 4350 3550
F 0 "R2" V 4250 3450 50  0000 C CNN
F 1 "500" V 4250 3600 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad1.05x0.95mm_HandSolder" V 4280 3550 50  0001 C CNN
F 3 "~" H 4350 3550 50  0001 C CNN
	1    4350 3550
	0    1    1    0   
$EndComp
Wire Wire Line
	4200 3550 4150 3550
$Comp
L Device:R R3
U 1 1 5DCFC46D
P 4350 3800
F 0 "R3" V 4250 3700 50  0000 C CNN
F 1 "500" V 4250 3850 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad1.05x0.95mm_HandSolder" V 4280 3800 50  0001 C CNN
F 3 "~" H 4350 3800 50  0001 C CNN
	1    4350 3800
	0    1    1    0   
$EndComp
Wire Wire Line
	4200 3800 4150 3800
$Comp
L Device:R R4
U 1 1 5DCFC51D
P 4350 4050
F 0 "R4" V 4250 3950 50  0000 C CNN
F 1 "500" V 4250 4100 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad1.05x0.95mm_HandSolder" V 4280 4050 50  0001 C CNN
F 3 "~" H 4350 4050 50  0001 C CNN
	1    4350 4050
	0    1    1    0   
$EndComp
Wire Wire Line
	4200 4050 4150 4050
Text Label 4050 3300 0    50   ~ 0
S_D1
Text Label 4050 3550 0    50   ~ 0
S_D2
Text Label 4050 3800 0    50   ~ 0
S_D3
$Comp
L Jumper:SolderJumper_2_Bridged JP1
U 1 1 5DCFF262
P 6850 2950
F 0 "JP1" V 6896 2862 50  0000 R CNN
F 1 "JP1" V 6805 2862 50  0000 R CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Bridged_Pad1.0x1.5mm" H 6850 2950 50  0001 C CNN
F 3 "~" H 6850 2950 50  0001 C CNN
	1    6850 2950
	0    -1   -1   0   
$EndComp
Connection ~ 6850 2800
Wire Wire Line
	6550 3300 6850 3300
Wire Wire Line
	6850 3300 6850 3100
Connection ~ 6850 3100
$Comp
L power:GND #PWR0103
U 1 1 5DD033DF
P 3350 2900
F 0 "#PWR0103" H 3350 2650 50  0001 C CNN
F 1 "GND" H 3355 2727 50  0000 C CNN
F 2 "" H 3350 2900 50  0001 C CNN
F 3 "" H 3350 2900 50  0001 C CNN
	1    3350 2900
	1    0    0    -1  
$EndComp
Wire Wire Line
	3000 2350 3400 2350
$Comp
L Device:C C1
U 1 1 5DD063E1
P 3350 2650
F 0 "C1" H 3465 2696 50  0000 L CNN
F 1 "68n" H 3465 2605 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 3388 2500 50  0001 C CNN
F 3 "~" H 3350 2650 50  0001 C CNN
	1    3350 2650
	1    0    0    -1  
$EndComp
Wire Wire Line
	3350 2900 3350 2800
Wire Wire Line
	3350 2500 3350 1950
Connection ~ 3350 1950
Wire Wire Line
	3350 1950 3400 1950
$Comp
L power:GND #PWR0104
U 1 1 5DD0CBC1
P 7650 2350
F 0 "#PWR0104" H 7650 2100 50  0001 C CNN
F 1 "GND" H 7655 2177 50  0000 C CNN
F 2 "" H 7650 2350 50  0001 C CNN
F 3 "" H 7650 2350 50  0001 C CNN
	1    7650 2350
	1    0    0    -1  
$EndComp
$Comp
L Device:C C2
U 1 1 5DD0CBC7
P 7650 2100
F 0 "C2" H 7765 2146 50  0000 L CNN
F 1 "68n" H 7765 2055 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 7688 1950 50  0001 C CNN
F 3 "~" H 7650 2100 50  0001 C CNN
	1    7650 2100
	1    0    0    -1  
$EndComp
Wire Wire Line
	7650 2350 7650 2250
Wire Wire Line
	7650 1950 6700 1950
Connection ~ 6700 1950
$EndSCHEMATC
