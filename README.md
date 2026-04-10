# SPACENOTIFY
SPACENOTIFY is a desktop dashboard that shows all space related activites including Launches, Landing and EVAs through the Launch Library 2 API.
# WHY
I designed SPACENOTIFY because I often can not look at my phone to check upcoming launches and I never want to miss one so this can sit on mydesk and as it has no controls can not be a distraction like a phone.
# Pictures

![Assembly](1.png)
![PCB](2.png)
![PCB](20260410_13h29m03s_grim.png)
![Mount](mount.png)
# BOM
| Name | Purpose | Quantity | Total Cost (USD) | Link | Distributor |
| --- | --- | --- | --- | --- | --- |
| GRM188R60J106KE47D 10uF capacitor | power filtering | 2 | 0.22 | https://www.digikey.com/en/products/detail/murata-electronics/GRM188R60J106KE47D/5797506 | Digikey |
| RC0402FR-075K1L 5.1k resistor | controll usb-c pd voltage | 2 | 0.20 | https://www.digikey.com/en/products/detail/yageo/RC0402FR-075K1L/726624 | Digikey |
| CX90BW1-24P1 USB c connector | power + data | 1 | 2.99 | https://www.digikey.com/en/products/detail/hirose-electric-co-ltd/CX90BW1-24P1/22566049?s=N4IgTCBcDaIMIA0CcAGAQgdQIwFowBYAFLEAXQF8g | Digikey |
| SKRPABE010 button | to trigger boot and reset the esp32 | 2 | 0.42 | https://www.digikey.com/en/products/detail/alps-alpine/SKRPABE010/18768948 | Digikey |
| TLV1117-33IDCYR | LDO voltage regulator | 1 | 0.45 | https://www.digikey.com/en/products/detail/texas-instruments/TLV1117-33IDCYR/1216719 | Digikey |
| ESP32-S3-WROOM-1-N8R2 | Main MCU | 1 | 5.71 | https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-WROOM-1-N8R2/15200058 | Digikey |
| Touch sensor | to allow UI interaction | 10 | 1.66 | https://www.aliexpress.us/item/3256806056841370.html?utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005006243156122%7C_p_origin_prod%3A&search_p4p_id=20260409181228570240878276160000137350_2 | Aliexpress |
| 1.8in LCD | Displays main info | 1 | 2.88 | https://www.aliexpress.us/item/3256805953674718.html?utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005006139989470%7C_p_origin_prod%3A | Aliexpress |
| PCB | Holds the display and esp32 | 1 | 5.00 |  | PCBWAY |
| I2c display | shows the info | 1 | 1.99 | https://www.aliexpress.us/item/3256805899077405.html?utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005006085392157%7C_p_origin_prod%3A&search_p4p_id=20260331193731576094732592880001235155_1 | Aliexpress |