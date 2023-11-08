EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "VKVM - VKM2"
Date "2023-06-08"
Rev "C"
Comp "Daniel Starke"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Regulator_Linear:XC6206PxxxMR U3
U 1 1 5E676B89
P 3400 4300
F 0 "U3" H 3400 4542 50  0000 C CNN
F 1 "XC6206P332MR" H 3400 4451 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23" H 3400 4500 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2304140030_Torex-Semicon-XC6206P332MR_C5446.pdf" H 3500 4050 50  0001 C CNN
F 4 "C5446" H 3400 4300 50  0001 C CNN "LCSC"
	1    3400 4300
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C8
U 1 1 5E684859
P 2850 4500
F 0 "C8" H 2942 4546 50  0000 L CNN
F 1 "100n" H 2942 4455 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 2850 4500 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 2850 4500 50  0001 C CNN
F 4 "C14663" H 2850 4500 50  0001 C CNN "LCSC"
	1    2850 4500
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C7
U 1 1 5E6852AD
P 2600 4500
F 0 "C7" H 2692 4546 50  0000 L CNN
F 1 "10u" H 2692 4455 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 2600 4500 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2304140030_Samsung-Electro-Mechanics-CL10A106KP8NNNC_C19702.pdf" H 2600 4500 50  0001 C CNN
F 4 "C19702" H 2600 4500 50  0001 C CNN "LCSC"
	1    2600 4500
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C11
U 1 1 5E6863FA
P 4350 4500
F 0 "C11" H 4442 4546 50  0000 L CNN
F 1 "10u" H 4442 4455 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 4350 4500 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2304140030_Samsung-Electro-Mechanics-CL10A106KP8NNNC_C19702.pdf" H 4350 4500 50  0001 C CNN
F 4 "C19702" H 4350 4500 50  0001 C CNN "LCSC"
	1    4350 4500
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C9
U 1 1 5E686400
P 3800 4500
F 0 "C9" H 3892 4546 50  0000 L CNN
F 1 "100n" H 3892 4455 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 3800 4500 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 3800 4500 50  0001 C CNN
F 4 "C14663" H 3800 4500 50  0001 C CNN "LCSC"
	1    3800 4500
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0103
U 1 1 5E686995
P 3400 4800
F 0 "#PWR0103" H 3400 4550 50  0001 C CNN
F 1 "GND" H 3405 4627 50  0000 C CNN
F 2 "" H 3400 4800 50  0001 C CNN
F 3 "" H 3400 4800 50  0001 C CNN
	1    3400 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 4600 2600 4750
Wire Wire Line
	3800 4750 3800 4600
Wire Wire Line
	2600 4750 2850 4750
Wire Wire Line
	3800 4750 4050 4750
Wire Wire Line
	4350 4750 4350 4600
Connection ~ 3800 4750
Wire Wire Line
	2850 4600 2850 4750
Connection ~ 2850 4750
Wire Wire Line
	2850 4750 3400 4750
Wire Wire Line
	3400 4600 3400 4750
Connection ~ 3400 4750
Wire Wire Line
	3400 4800 3400 4750
Wire Wire Line
	3800 4400 3800 4300
Wire Wire Line
	3800 4300 3700 4300
Wire Wire Line
	4350 4400 4350 4300
Wire Wire Line
	4050 4300 3800 4300
Connection ~ 3800 4300
Wire Wire Line
	3100 4300 2850 4300
Wire Wire Line
	2850 4300 2850 4400
Wire Wire Line
	2850 4300 2700 4300
Connection ~ 2850 4300
$Comp
L Device:C_Small C10
U 1 1 5E68CEF3
P 4050 4500
F 0 "C10" H 4142 4546 50  0000 L CNN
F 1 "100n" H 4142 4455 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 4050 4500 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 4050 4500 50  0001 C CNN
F 4 "C14663" H 4050 4500 50  0001 C CNN "LCSC"
	1    4050 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	4050 4400 4050 4300
Wire Wire Line
	4050 4600 4050 4750
Wire Wire Line
	4350 4300 4050 4300
Connection ~ 4050 4300
Wire Wire Line
	4050 4750 4350 4750
Connection ~ 4050 4750
$Comp
L Connector:USB_A J4
U 1 1 5E6951D7
P 5950 4400
F 0 "J4" H 6007 4867 50  0000 C CNN
F 1 "USB_A" H 6007 4776 50  0000 C CNN
F 2 "Connector_USB:USB_A_TH_AM90" H 6100 4350 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811131825_Jing-Extension-of-the-Electronic-Co-C9739_C9739.pdf" H 6100 4350 50  0001 C CNN
F 4 "C9739" H 5950 4400 50  0001 C CNN "LCSC"
	1    5950 4400
	1    0    0    -1  
$EndComp
Text GLabel 6800 2700 2    50   Input ~ 0
USB1_D-
Text GLabel 6250 4500 2    50   Input ~ 0
USB1_D-
Text GLabel 6800 2800 2    50   Input ~ 0
USB1_D+
Text GLabel 6250 4400 2    50   Input ~ 0
USB1_D+
$Comp
L power:GND #PWR0104
U 1 1 5E69D025
P 5950 5000
F 0 "#PWR0104" H 5950 4750 50  0001 C CNN
F 1 "GND" H 5955 4827 50  0000 C CNN
F 2 "" H 5950 5000 50  0001 C CNN
F 3 "" H 5950 5000 50  0001 C CNN
	1    5950 5000
	1    0    0    -1  
$EndComp
Wire Wire Line
	5950 4800 5950 5000
Wire Wire Line
	5850 5000 5950 5000
Connection ~ 5950 5000
Text GLabel 6250 4200 2    50   Input ~ 0
USB1_VBUS
Text GLabel 1500 4300 0    50   Input ~ 0
USB1_VBUS
$Comp
L power:+3.3V #PWR0105
U 1 1 5E6A8E24
P 4350 4300
F 0 "#PWR0105" H 4350 4150 50  0001 C CNN
F 1 "+3.3V" H 4365 4473 50  0000 C CNN
F 2 "" H 4350 4300 50  0001 C CNN
F 3 "" H 4350 4300 50  0001 C CNN
	1    4350 4300
	1    0    0    -1  
$EndComp
Connection ~ 4350 4300
$Comp
L power:+3.3V #PWR0106
U 1 1 5E6A9377
P 6000 1600
F 0 "#PWR0106" H 6000 1450 50  0001 C CNN
F 1 "+3.3V" H 6015 1773 50  0000 C CNN
F 2 "" H 6000 1600 50  0001 C CNN
F 3 "" H 6000 1600 50  0001 C CNN
	1    6000 1600
	1    0    0    -1  
$EndComp
Wire Wire Line
	6000 1600 6000 1650
Wire Wire Line
	6100 1700 6100 1650
Wire Wire Line
	6100 1650 6000 1650
Connection ~ 6000 1650
Wire Wire Line
	6000 1650 6000 1700
$Comp
L power:+3.3V #PWR0107
U 1 1 5E6ABD5C
P 9300 1600
F 0 "#PWR0107" H 9300 1450 50  0001 C CNN
F 1 "+3.3V" H 9315 1773 50  0000 C CNN
F 2 "" H 9300 1600 50  0001 C CNN
F 3 "" H 9300 1600 50  0001 C CNN
	1    9300 1600
	1    0    0    -1  
$EndComp
Wire Wire Line
	9300 1600 9300 1650
Wire Wire Line
	9400 1700 9400 1650
Wire Wire Line
	9400 1650 9300 1650
Connection ~ 9300 1650
Wire Wire Line
	9300 1650 9300 1700
$Comp
L Device:R_Small R6
U 1 1 5E6BB680
P 1850 4900
F 0 "R6" H 1791 4854 50  0000 R CNN
F 1 "2.2k" H 1791 4945 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1850 4900 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811021210_UNI-ROYAL-Uniroyal-Elec-0603WAF2201T5E_C4190.pdf" H 1850 4900 50  0001 C CNN
F 4 "C4190" H 1850 4900 50  0001 C CNN "LCSC"
	1    1850 4900
	-1   0    0    1   
$EndComp
Text GLabel 1600 5150 3    50   Input ~ 0
USB1_VBUS_SENSE
$Comp
L Device:R_Small R5
U 1 1 5E6F5D96
P 1600 4900
F 0 "R5" H 1541 4854 50  0000 R CNN
F 1 "2.2k" H 1541 4945 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1600 4900 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811021210_UNI-ROYAL-Uniroyal-Elec-0603WAF2201T5E_C4190.pdf" H 1600 4900 50  0001 C CNN
F 4 "C4190" H 1600 4900 50  0001 C CNN "LCSC"
	1    1600 4900
	-1   0    0    1   
$EndComp
Text GLabel 1850 5150 3    50   Input ~ 0
USB2_VBUS_SENSE
Text GLabel 1500 4600 0    50   Input ~ 0
USB2_VBUS
Wire Wire Line
	1500 4300 1600 4300
Wire Wire Line
	1600 4300 1600 4800
Connection ~ 1600 4300
Wire Wire Line
	1600 4300 1950 4300
Wire Wire Line
	1500 4600 1850 4600
Wire Wire Line
	1850 4600 1850 4800
Connection ~ 1850 4600
Wire Wire Line
	1850 4600 1950 4600
Wire Wire Line
	1600 5150 1600 5000
Wire Wire Line
	1850 5150 1850 5000
Text GLabel 10100 2700 2    50   Input ~ 0
USB2_D-
Text GLabel 10100 2800 2    50   Input ~ 0
USB2_D+
Wire Wire Line
	2250 4300 2450 4300
Wire Wire Line
	2250 4600 2450 4600
Wire Wire Line
	2450 4300 2600 4300
Connection ~ 2450 4300
Connection ~ 2700 4300
Text GLabel 9650 4150 2    50   Input ~ 0
USB2_VBUS
$Comp
L power:GND #PWR0109
U 1 1 5E75B891
P 9050 5850
F 0 "#PWR0109" H 9050 5600 50  0001 C CNN
F 1 "GND" H 9055 5677 50  0000 C CNN
F 2 "" H 9050 5850 50  0001 C CNN
F 3 "" H 9050 5850 50  0001 C CNN
	1    9050 5850
	1    0    0    -1  
$EndComp
Wire Wire Line
	9050 5650 9050 5850
Wire Wire Line
	8750 5850 9050 5850
Connection ~ 9050 5850
Text GLabel 9850 4850 2    50   Input ~ 0
USB2_D+
Text GLabel 9850 4750 2    50   Input ~ 0
USB2_D-
Wire Wire Line
	9650 4650 9750 4650
Wire Wire Line
	9750 4650 9750 4750
Wire Wire Line
	9750 4750 9850 4750
Wire Wire Line
	9750 4750 9650 4750
Connection ~ 9750 4750
Wire Wire Line
	9650 4850 9750 4850
Wire Wire Line
	9650 4950 9750 4950
Wire Wire Line
	9750 4950 9750 4850
Connection ~ 9750 4850
Wire Wire Line
	9750 4850 9850 4850
NoConn ~ 9650 5350
NoConn ~ 9650 5250
$Comp
L Device:R_Small R1
U 1 1 5E7881B7
P 9950 4350
F 0 "R1" V 10146 4350 50  0000 C CNN
F 1 "5.1k" V 10055 4350 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 9950 4350 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811021212_UNI-ROYAL-Uniroyal-Elec-0603WAF5101T5E_C23186.pdf" H 9950 4350 50  0001 C CNN
F 4 "C23186" H 9950 4350 50  0001 C CNN "LCSC"
	1    9950 4350
	0    -1   -1   0   
$EndComp
$Comp
L Device:R_Small R2
U 1 1 5E7889C1
P 9950 4450
F 0 "R2" V 10146 4450 50  0000 C CNN
F 1 "5.1k" V 10055 4450 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 9950 4450 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811021212_UNI-ROYAL-Uniroyal-Elec-0603WAF5101T5E_C23186.pdf" H 9950 4450 50  0001 C CNN
F 4 "C23186" H 9950 4450 50  0001 C CNN "LCSC"
	1    9950 4450
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0110
U 1 1 5E788BAE
P 10450 4500
F 0 "#PWR0110" H 10450 4250 50  0001 C CNN
F 1 "GND" H 10455 4327 50  0000 C CNN
F 2 "" H 10450 4500 50  0001 C CNN
F 3 "" H 10450 4500 50  0001 C CNN
	1    10450 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	10450 4350 10450 4450
Connection ~ 10450 4450
Wire Wire Line
	10450 4450 10450 4500
Wire Wire Line
	10050 4350 10450 4350
Wire Wire Line
	9850 4350 9650 4350
Wire Wire Line
	9650 4450 9850 4450
Text GLabel 10100 2300 2    50   Input ~ 0
USB2_VBUS_SENSE
Text Label 6850 2100 0    50   ~ 0
USART2_TX
Text Label 6850 2200 0    50   ~ 0
USART2_RX
$Comp
L power:GND #PWR0113
U 1 1 5E6F06BA
P 6000 3200
F 0 "#PWR0113" H 6000 2950 50  0001 C CNN
F 1 "GND" H 6005 3027 50  0000 C CNN
F 2 "" H 6000 3200 50  0001 C CNN
F 3 "" H 6000 3200 50  0001 C CNN
	1    6000 3200
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0114
U 1 1 5E6F09BC
P 9300 3200
F 0 "#PWR0114" H 9300 2950 50  0001 C CNN
F 1 "GND" H 9305 3027 50  0000 C CNN
F 2 "" H 9300 3200 50  0001 C CNN
F 3 "" H 9300 3200 50  0001 C CNN
	1    9300 3200
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG0103
U 1 1 5E687A18
P 9650 4150
F 0 "#FLG0103" H 9650 4225 50  0001 C CNN
F 1 "PWR_FLAG" H 9650 4323 50  0000 C CNN
F 2 "" H 9650 4150 50  0001 C CNN
F 3 "~" H 9650 4150 50  0001 C CNN
	1    9650 4150
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG0104
U 1 1 5E687EF5
P 6250 4200
F 0 "#FLG0104" H 6250 4275 50  0001 C CNN
F 1 "PWR_FLAG" H 6250 4373 50  0000 C CNN
F 2 "" H 6250 4200 50  0001 C CNN
F 3 "~" H 6250 4200 50  0001 C CNN
	1    6250 4200
	1    0    0    -1  
$EndComp
NoConn ~ 10100 2400
NoConn ~ 10100 2500
NoConn ~ 10100 2600
NoConn ~ 6800 2600
NoConn ~ 6800 2500
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 5E6E5150
P 10450 4350
F 0 "#FLG0102" H 10450 4425 50  0001 C CNN
F 1 "PWR_FLAG" H 10450 4523 50  0000 C CNN
F 2 "" H 10450 4350 50  0001 C CNN
F 3 "~" H 10450 4350 50  0001 C CNN
	1    10450 4350
	1    0    0    -1  
$EndComp
Connection ~ 10450 4350
Text GLabel 6800 2300 2    50   Input ~ 0
USB1_VBUS_SENSE
NoConn ~ 5600 2700
$Comp
L MCU_ST_STM32F0:STM32F042F6Px U1
U 1 1 5E6DC733
P 6200 2400
F 0 "U1" H 6200 1650 50  0000 C CNN
F 1 "STM32F042F6Px" H 6450 1550 50  0000 C CNN
F 2 "Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm" H 5700 1700 50  0001 R CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00105814.pdf" H 6200 2400 50  0001 C CNN
F 4 "C81000" H 6200 2400 50  0001 C CNN "LCSC"
	1    6200 2400
	1    0    0    -1  
$EndComp
$Comp
L MCU_ST_STM32F0:STM32F042F6Px U2
U 1 1 5E6DF307
P 9500 2400
F 0 "U2" H 9500 1511 50  0000 C CNN
F 1 "STM32F042F6Px" H 9500 1420 50  0000 C CNN
F 2 "Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm" H 9000 1700 50  0001 R CNN
F 3 "http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/DM00105814.pdf" H 9500 2400 50  0001 C CNN
F 4 "C81000" H 9500 2400 50  0001 C CNN "LCSC"
	1    9500 2400
	1    0    0    -1  
$EndComp
Wire Wire Line
	6800 2100 7850 2100
Wire Wire Line
	7850 1250 7850 2100
Wire Wire Line
	6800 2200 7950 2200
Wire Wire Line
	7950 1350 7950 2200
NoConn ~ 6800 1900
NoConn ~ 6800 2000
NoConn ~ 5600 2600
NoConn ~ 5600 2900
NoConn ~ 8900 2600
NoConn ~ 8900 2900
NoConn ~ 10100 1900
NoConn ~ 10100 2000
Text GLabel 5400 1900 0    50   Input ~ 0
RESET1
Text GLabel 8700 1900 0    50   Input ~ 0
RESET2
$Comp
L Device:R_Small R3
U 1 1 5E731F62
P 5600 1800
F 0 "R3" H 5541 1754 50  0000 R CNN
F 1 "10k" H 5541 1845 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 5600 1800 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811062009_UNI-ROYAL-Uniroyal-Elec-0603WAF1002T5E_C25804.pdf" H 5600 1800 50  0001 C CNN
F 4 "C25804" H 5600 1800 50  0001 C CNN "LCSC"
	1    5600 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	6000 1650 5600 1650
Wire Wire Line
	5600 1650 5600 1700
Wire Wire Line
	5400 1900 5450 1900
Connection ~ 5600 1900
$Comp
L Device:R_Small R7
U 1 1 5E73F824
P 8900 1800
F 0 "R7" H 8841 1754 50  0000 R CNN
F 1 "10k" H 8841 1845 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 8900 1800 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811062009_UNI-ROYAL-Uniroyal-Elec-0603WAF1002T5E_C25804.pdf" H 8900 1800 50  0001 C CNN
F 4 "C25804" H 8900 1800 50  0001 C CNN "LCSC"
	1    8900 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	9300 1650 8900 1650
Wire Wire Line
	8900 1650 8900 1700
Wire Wire Line
	8700 1900 8750 1900
Connection ~ 8900 1900
$Comp
L Device:R_Small R4
U 1 1 5E746787
P 5600 3100
F 0 "R4" H 5541 3054 50  0000 R CNN
F 1 "10k" H 5541 3145 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 5600 3100 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811062009_UNI-ROYAL-Uniroyal-Elec-0603WAF1002T5E_C25804.pdf" H 5600 3100 50  0001 C CNN
F 4 "C25804" H 5600 3100 50  0001 C CNN "LCSC"
	1    5600 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	5400 3000 5450 3000
Connection ~ 5600 3000
Wire Wire Line
	5600 3200 6000 3200
Connection ~ 6000 3200
$Comp
L Device:R_Small R8
U 1 1 5E752A90
P 8900 3100
F 0 "R8" H 8841 3054 50  0000 R CNN
F 1 "10k" H 8841 3145 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 8900 3100 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811062009_UNI-ROYAL-Uniroyal-Elec-0603WAF1002T5E_C25804.pdf" H 8900 3100 50  0001 C CNN
F 4 "C25804" H 8900 3100 50  0001 C CNN "LCSC"
	1    8900 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	8700 3000 8750 3000
Connection ~ 8900 3000
Wire Wire Line
	8900 3200 9300 3200
Connection ~ 9300 3200
$Comp
L Device:C_Small C2
U 1 1 5E7DF30E
P 5450 1800
F 0 "C2" H 5542 1846 50  0000 L CNN
F 1 "100n" H 5542 1755 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 5450 1800 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 5450 1800 50  0001 C CNN
F 4 "C14663" H 5450 1800 50  0001 C CNN "LCSC"
	1    5450 1800
	1    0    0    -1  
$EndComp
Connection ~ 5450 1900
Wire Wire Line
	5450 1900 5600 1900
Wire Wire Line
	5600 1650 5450 1650
Wire Wire Line
	5450 1650 5450 1700
Connection ~ 5600 1650
Text GLabel 5400 3000 0    50   Input ~ 0
BOOT1
Text GLabel 8700 3000 0    50   Input ~ 0
BOOT2
$Comp
L Device:C_Small C3
U 1 1 5E7F4222
P 5450 3100
F 0 "C3" H 5542 3146 50  0000 L CNN
F 1 "100n" H 5542 3055 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 5450 3100 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 5450 3100 50  0001 C CNN
F 4 "C14663" H 5450 3100 50  0001 C CNN "LCSC"
	1    5450 3100
	1    0    0    -1  
$EndComp
Connection ~ 5450 3000
Wire Wire Line
	5450 3000 5600 3000
Wire Wire Line
	5600 3200 5450 3200
Connection ~ 5600 3200
$Comp
L Device:C_Small C6
U 1 1 5E7F7805
P 8750 3100
F 0 "C6" H 8842 3146 50  0000 L CNN
F 1 "100n" H 8842 3055 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 8750 3100 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 8750 3100 50  0001 C CNN
F 4 "C14663" H 8750 3100 50  0001 C CNN "LCSC"
	1    8750 3100
	1    0    0    -1  
$EndComp
Connection ~ 8750 3000
Wire Wire Line
	8750 3000 8900 3000
$Comp
L Device:C_Small C5
U 1 1 5E7F7DB4
P 8750 1800
F 0 "C5" H 8842 1846 50  0000 L CNN
F 1 "100n" H 8842 1755 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 8750 1800 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1809301912_YAGEO-CC0603KRX7R9BB104_C14663.pdf" H 8750 1800 50  0001 C CNN
F 4 "C14663" H 8750 1800 50  0001 C CNN "LCSC"
	1    8750 1800
	1    0    0    -1  
$EndComp
Connection ~ 8750 1900
Wire Wire Line
	8750 1900 8900 1900
Wire Wire Line
	8900 1650 8750 1650
Wire Wire Line
	8750 1650 8750 1700
Connection ~ 8900 1650
Wire Wire Line
	8750 3200 8900 3200
Connection ~ 8900 3200
Wire Wire Line
	10200 1350 10200 2100
Wire Wire Line
	10200 2100 10100 2100
Wire Wire Line
	10300 2200 10100 2200
Wire Wire Line
	10300 1250 10300 2200
$Comp
L Device:D_Schottky D1
U 1 1 6474F4B1
P 2100 4300
F 0 "D1" H 2100 4084 50  0000 C CNN
F 1 "B5819WSL" H 2100 4175 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123" H 2100 4300 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2304140030_Jiangsu-Changjing-Electronics-Technology-Co---Ltd--B5819W-SL_C8598.pdf" H 2100 4300 50  0001 C CNN
F 4 "C8598" H 2100 4300 50  0001 C CNN "LCSC"
	1    2100 4300
	-1   0    0    1   
$EndComp
$Comp
L Device:D_Schottky D2
U 1 1 64750AE9
P 2100 4600
F 0 "D2" H 2100 4384 50  0000 C CNN
F 1 "B5819WSL" H 2100 4475 50  0000 C CNN
F 2 "Diode_SMD:D_SOD-123" H 2100 4600 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2304140030_Jiangsu-Changjing-Electronics-Technology-Co---Ltd--B5819W-SL_C8598.pdf" H 2100 4600 50  0001 C CNN
F 4 "C8598" H 2100 4600 50  0001 C CNN "LCSC"
	1    2100 4600
	-1   0    0    1   
$EndComp
Wire Wire Line
	2450 4300 2450 4600
Text GLabel 6800 3000 2    50   Input ~ 0
SWCLK1
Text GLabel 6800 2900 2    50   Input ~ 0
SWDIO1
Text GLabel 10100 2900 2    50   Input ~ 0
SWDIO2
Text GLabel 10100 3000 2    50   Input ~ 0
SWCLK2
Text GLabel 2450 1850 2    50   Input ~ 0
RESET1
Text GLabel 2450 1950 2    50   Input ~ 0
BOOT1
Text GLabel 2450 2050 2    50   Input ~ 0
SWDIO1
Text GLabel 2450 2150 2    50   Input ~ 0
SWCLK1
Text GLabel 2450 2650 2    50   Input ~ 0
RESET2
Text GLabel 2450 2550 2    50   Input ~ 0
BOOT2
Text GLabel 2450 2450 2    50   Input ~ 0
SWDIO2
Text GLabel 2450 2350 2    50   Input ~ 0
SWCLK2
$Comp
L Device:R_Small R10
U 1 1 64781A20
P 8750 5750
F 0 "R10" H 8691 5704 50  0000 R CNN
F 1 "33k" H 8691 5795 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 8750 5750 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2206010230_UNI-ROYAL-Uniroyal-Elec-0603WAF3302T5E_C4216.pdf" H 8750 5750 50  0001 C CNN
F 4 "C4216" H 8750 5750 50  0001 C CNN "LCSC"
	1    8750 5750
	1    0    0    -1  
$EndComp
$Comp
L Device:R_Small R9
U 1 1 64782097
P 5850 4900
F 0 "R9" H 5791 4854 50  0000 R CNN
F 1 "33k" H 5791 4945 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 5850 4900 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/2206010230_UNI-ROYAL-Uniroyal-Elec-0603WAF3302T5E_C4216.pdf" H 5850 4900 50  0001 C CNN
F 4 "C4216" H 5850 4900 50  0001 C CNN "LCSC"
	1    5850 4900
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0101
U 1 1 647A2DF3
P 3050 2800
F 0 "#PWR0101" H 3050 2550 50  0001 C CNN
F 1 "GND" H 3055 2627 50  0000 C CNN
F 2 "" H 3050 2800 50  0001 C CNN
F 3 "" H 3050 2800 50  0001 C CNN
	1    3050 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2450 2250 3050 2250
Wire Wire Line
	10050 4450 10450 4450
$Comp
L Connector:USB_C_Receptacle_USB2.0 J1
U 1 1 647FBB45
P 9050 4750
F 0 "J1" H 9157 5617 50  0000 C CNN
F 1 "USB_C_Receptacle_USB2.0" H 9157 5526 50  0000 C CNN
F 2 "Connector_USB:USB_C_HRO_TYPE_C_31_M_12" H 9200 4750 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811131825_Korean-Hroparts-Elec-TYPE-C-31-M-12_C165948.pdf" H 9200 4750 50  0001 C CNN
F 4 "C165948" H 9050 4750 50  0001 C CNN "LCSC"
	1    9050 4750
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x09 J2
U 1 1 647C8283
P 2250 2250
F 0 "J2" H 2168 2867 50  0000 C CNN
F 1 "Conn_01x09" H 2168 2776 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x09_P2.54mm_Vertical" H 2250 2250 50  0001 C CNN
F 3 "~" H 2250 2250 50  0001 C CNN
	1    2250 2250
	-1   0    0    -1  
$EndComp
Wire Wire Line
	3050 2250 3050 2800
Text Notes 4850 1100 0    50   ~ 0
Controller
Text Notes 7800 1100 0    50   ~ 0
Periphery
Text Notes 900  1100 0    50   ~ 0
Debug
Wire Notes Line
	4800 3550 850  3550
Wire Notes Line
	4800 1000 4800 6200
Wire Notes Line
	7750 1000 7750 6200
Wire Notes Line
	11000 1000 11000 6200
Text Notes 2950 1800 3    50   ~ 0
Controller
Text Notes 2950 2350 3    50   ~ 0
Periphery
Text Notes 900  3650 0    50   ~ 0
Power Supply
Wire Notes Line
	850  1000 850  6200
Wire Notes Line
	850  6200 11000 6200
$Comp
L Connector:TestPoint TP3
U 1 1 64875CD5
P 1600 3900
F 0 "TP3" H 1658 4018 50  0000 L CNN
F 1 "USB1" H 1658 3927 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 1800 3900 50  0001 C CNN
F 3 "~" H 1800 3900 50  0001 C CNN
	1    1600 3900
	1    0    0    -1  
$EndComp
$Comp
L Connector:TestPoint TP4
U 1 1 64876F13
P 1850 3900
F 0 "TP4" H 1908 4018 50  0000 L CNN
F 1 "USB2" H 1908 3927 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 2050 3900 50  0001 C CNN
F 3 "~" H 2050 3900 50  0001 C CNN
	1    1850 3900
	1    0    0    -1  
$EndComp
Wire Wire Line
	1600 3900 1600 4300
Wire Wire Line
	1850 4600 1850 3900
$Comp
L Connector:TestPoint TP5
U 1 1 6487CC04
P 2850 3900
F 0 "TP5" H 2908 4018 50  0000 L CNN
F 1 "5V" H 2908 3927 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 3050 3900 50  0001 C CNN
F 3 "~" H 3050 3900 50  0001 C CNN
	1    2850 3900
	1    0    0    -1  
$EndComp
Wire Wire Line
	2850 3900 2850 4300
$Comp
L Connector:TestPoint TP6
U 1 1 6487FFB6
P 4050 3900
F 0 "TP6" H 4108 4018 50  0000 L CNN
F 1 "3V3" H 4108 3927 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 4250 3900 50  0001 C CNN
F 3 "~" H 4250 3900 50  0001 C CNN
	1    4050 3900
	1    0    0    -1  
$EndComp
Wire Wire Line
	4050 3900 4050 4300
Text GLabel 6800 2400 2    50   Input ~ 0
USB2_VBUS_SENSE
$Comp
L power:+5V #PWR02
U 1 1 64885495
P 2700 4300
F 0 "#PWR02" H 2700 4150 50  0001 C CNN
F 1 "+5V" H 2715 4473 50  0000 C CNN
F 2 "" H 2700 4300 50  0001 C CNN
F 3 "" H 2700 4300 50  0001 C CNN
	1    2700 4300
	1    0    0    -1  
$EndComp
$Comp
L Device:LED D3
U 1 1 6488773B
P 8200 2100
F 0 "D3" V 8239 1982 50  0000 R CNN
F 1 "GREEN" V 8148 1982 50  0000 R CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 8200 2100 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/szlcsc/1811101510_Everlight-Elec-19-217-GHC-YR1S2-3T_C72043.pdf" H 8200 2100 50  0001 C CNN
F 4 "C72043" V 8200 2100 50  0001 C CNN "LCSC"
	1    8200 2100
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R11
U 1 1 648888F1
P 8200 2400
F 0 "R11" H 8270 2446 50  0000 L CNN
F 1 "820R" H 8270 2355 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 8130 2400 50  0001 C CNN
F 3 "https://datasheet.lcsc.com/lcsc/1811101510_Everlight-Elec-19-217-GHC-YR1S2-3T_C72043.pdf" H 8200 2400 50  0001 C CNN
F 4 "C23253" H 8200 2400 50  0001 C CNN "LCSC"
	1    8200 2400
	1    0    0    -1  
$EndComp
Wire Wire Line
	8900 2700 8200 2700
Wire Wire Line
	8200 2700 8200 2550
Text GLabel 8200 1900 1    50   Input ~ 0
USB2_VBUS
Wire Wire Line
	8200 1950 8200 1900
$Comp
L Connector:TestPoint TP7
U 1 1 648D0105
P 4350 4750
F 0 "TP7" H 4300 4900 50  0000 R CNN
F 1 "GND" H 4300 5000 50  0000 R CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 4550 4750 50  0001 C CNN
F 3 "~" H 4550 4750 50  0001 C CNN
	1    4350 4750
	-1   0    0    1   
$EndComp
Connection ~ 4350 4750
Wire Wire Line
	3400 4750 3800 4750
Wire Notes Line
	850  1000 11000 1000
Wire Wire Line
	7950 1350 10200 1350
Wire Wire Line
	7850 1250 10300 1250
Connection ~ 10300 1250
Connection ~ 10200 1350
$Comp
L Connector_Generic:Conn_01x03 J3
U 1 1 64867412
P 10750 1350
F 0 "J3" H 10668 1025 50  0000 C CNN
F 1 "Conn_01x03" H 10668 1116 50  0000 C CNN
F 2 "vkm-c:PinHeader_1x03_P2.54mm_Horizontal_Edge" H 10750 1350 50  0001 C CNN
F 3 "~" H 10750 1350 50  0001 C CNN
	1    10750 1350
	1    0    0    1   
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 64867C40
P 10500 1500
F 0 "#PWR0102" H 10500 1250 50  0001 C CNN
F 1 "GND" H 10505 1327 50  0000 C CNN
F 2 "" H 10500 1500 50  0001 C CNN
F 3 "" H 10500 1500 50  0001 C CNN
	1    10500 1500
	1    0    0    -1  
$EndComp
Wire Wire Line
	10300 1250 10550 1250
Wire Wire Line
	10200 1350 10550 1350
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 64890C0F
P 2450 4300
F 0 "#FLG0101" H 2450 4375 50  0001 C CNN
F 1 "PWR_FLAG" H 2450 4473 50  0000 C CNN
F 2 "" H 2450 4300 50  0001 C CNN
F 3 "~" H 2450 4300 50  0001 C CNN
	1    2450 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	2600 4400 2600 4300
Connection ~ 2600 4300
Wire Wire Line
	2600 4300 2700 4300
Wire Wire Line
	10550 1450 10500 1450
Wire Wire Line
	10500 1450 10500 1500
$EndSCHEMATC
