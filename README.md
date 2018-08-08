# energy-monitoring

Source for my home's energy monitoring system

- [arduino](arduino) contains the firmware for an Arduino UNO board that samples voltage and intensity signals. Sampled data can be transferred to the computer through the USB port as a serial device.
- [python](python) contains Python code that knows how to read voltage and intensity data sampled with the Arduino board, how to filter and process the data to calculate Volts, Intensity and Power, and generate plots with [matplotlib](https://matplotlib.org/).
- [eagle](eagle) contains a hardware design for the final energy monitoring system, including the analog stages to prepare voltage and intensity signals to be sampled by the Arduino ADC (Unfinished).

# Blogs

* [My home energy monitoring vision](https://aloneinthehack.wordpress.com/2013/05/20/my-home-energy-monitoring-vision/)
* [First power measurements](https://aloneinthehack.wordpress.com/2013/04/27/first-power-measurements/)
* [Starting to explore OpenEnergyMonitor.org](https://aloneinthehack.wordpress.com/2013/04/07/starting-to-explore-openenergymonitor-org/)
