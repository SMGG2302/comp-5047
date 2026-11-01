# Nest microclimate: 10-day simulation

Open [index.html](index.html) in a browser to view the English temperature and relative-humidity charts and select a day. The page is standalone and requires no server or API key. The gray area marks the 09:00–17:00 observation window. Days 4 and 8 are cloudy; the other eight days are sunny.

## Data files

- [Excel workbook](nest-microclimate-10-days.xlsx): daily comparisons, continuous data, and model assumptions (worksheet labels are in Chinese).
- [Full-day CSV](full-day-5-minute-data.csv): 2,880 records, one every five minutes over ten days.
- [Observation-window CSV](observation-window-5-minute-data.csv): 960 records, 09:00–16:55 each day, including the two cloudy days.

All values are synthetic, not field measurements or evidence of system effectiveness. The comparison includes ambient conditions, a standard nest box, a hypothetical natural hollow, an artificial hollow without dripping, and the same artificial hollow with drip control.

The model uses a one-minute thermal-response calculation with assumed time constants and cooling capacity. It assumes no additional internal water-vapour source from the isolated water layer. Relative humidity can still increase as temperature falls. The workbook documents the parameters and limitations. Neither similarity to natural hollows nor humidity equivalence has been validated experimentally.

The simulated controller opens above 30°C when the synthetic light gate is satisfied, then checks every minute and closes below 26°C. These assumptions can produce several hours of continuous opening on hot days. The light gate is not a calibrated conversion from the ESP32 light ADC, and servo travel is omitted.
