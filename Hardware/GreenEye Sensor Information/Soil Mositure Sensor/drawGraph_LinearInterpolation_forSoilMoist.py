import numpy as np
import matplotlib.pyplot as plt

# 1. Define calibration constants
DRY_VALUE = 1719  # ADC value at 0% moisture
WET_VALUE = 1662  # ADC value at 100% moisture

# 2. Function to convert ADC value to moisture percentage
def get_soil_moisture_percent(adc_value):
    # Linear conversion formula
    moisture_percent = (float)(DRY_VALUE - adc_value) * 100.0 / (float)(DRY_VALUE - WET_VALUE)
  
    # Clamp the result between 0% and 100%
    if moisture_percent > 100.0:
        moisture_percent = 100.0
    if moisture_percent < 0.0:
        moisture_percent = 0.0
    
    return moisture_percent

# 3. Generate data for plotting
# Set ADC range slightly wider than calibration points
adc_range = np.linspace(WET_VALUE - 20, DRY_VALUE + 20, 500)
moisture_range = [get_soil_moisture_percent(v) for v in adc_range]

# 4. Plotting
fig, ax = plt.subplots(figsize=(12, 8))

# Plot the linear relationship
ax.plot(adc_range, moisture_range, color='dodgerblue', linewidth=2.5, label='Linear Conversion')

# Plot the WET and DRY calibration points
ax.plot(WET_VALUE, 100, 'o', color='blue', markersize=12, label=f'WET Point ({WET_VALUE})')
ax.plot(DRY_VALUE, 0, 'o', color='saddlebrown', markersize=12, label=f'DRY Point ({DRY_VALUE})')

# Add text annotations for the points, now including ADC values
ax.text(WET_VALUE, 90, f'WET\n(ADC: {WET_VALUE}, 100%)', ha='center', va='top', fontsize=11, color='blue', weight='bold')
ax.text(DRY_VALUE, 10, f'DRY\n(ADC: {DRY_VALUE}, 0%)', ha='center', va='bottom', fontsize=11, color='saddlebrown', weight='bold')

# 5. Add the formula box
# Use LaTeX for a clean mathematical look
formula_text = (
    r"$\bf{Conversion\ Formula}$" + "\n\n"
    fr"$Moisture(\%) = \frac{{DRY\_VALUE - ADC}}{{DRY\_VALUE - WET\_VALUE}} \times 100$" + "\n\n"
    fr"$Moisture(\%) = \frac{{{DRY_VALUE} - ADC}}{{{DRY_VALUE - WET_VALUE}}} \times 100$"
)
ax.text(0.95, 0.95, formula_text, transform=ax.transAxes, fontsize=12,
        verticalalignment='top', horizontalalignment='right',
        bbox=dict(boxstyle='round,pad=0.5', fc='wheat', alpha=0.5))

# 6. Customize the plot
ax.set_title('Green-Eye Sensor Device Soil Moisture Sensor', fontsize=16)
ax.set_xlabel('Sensor ADC Value (12-bit)', fontsize=12)
ax.set_ylabel('Soil Moisture (%)', fontsize=12)
ax.grid(True, linestyle='--', alpha=0.6)
ax.legend(fontsize=11)

# Set axis limits
ax.set_xlim(WET_VALUE - 20, DRY_VALUE + 20)
ax.set_ylim(-10, 110)

# 7. Save the file
plt.tight_layout()
plt.savefig('green-eye_soil_moisture_graph.png')

print("Graph has been saved.")
