import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline
from matplotlib.lines import Line2D

# 1. Data points
x_coords = np.array([1920, 1987, 3128])
y_coords = np.array([84, 1413, 12880])

# 2. Create spline function
cs = CubicSpline(x_coords, y_coords)

# 3. Calculate and store all coefficients for both formula types
compact_coeffs = []
direct_coeffs = []
for i in range(len(x_coords) - 1):
    # Store compact coefficients: a(x-k)^3 + ...
    a, b, c, d = cs.c[:, i]
    compact_coeffs.append({'a': a, 'b': b, 'c': c, 'd': d})
    
    # Calculate and store direct coefficients: Ax^3 + ...
    k = x_coords[i]
    A = a
    B = -3*a*k + b
    C = 3*a*k**2 - 2*b*k + c
    D = -a*k**3 + b*k**2 - c*k + d
    direct_coeffs.append({'A': A, 'B': B, 'C': C, 'D': D})

# 4. Plotting setup
fig, ax = plt.subplots(figsize=(15, 12))

# Plot curve segments with different colors
curve_colors = ['blue', 'darkorange']
for i in range(len(x_coords) - 1):
    x_segment = np.linspace(x_coords[i], x_coords[i+1], 250)
    y_segment = cs(x_segment)
    ax.plot(x_segment, y_segment, color=curve_colors[i], linestyle='-', linewidth=2.5)

# Plot each point and adjust text position
point_colors = ['red', 'green', 'purple'] 
for i in range(len(x_coords)):
    ax.plot(x_coords[i], y_coords[i], 'o', color=point_colors[i], markersize=10, zorder=5)
    ax.text(x_coords[i], y_coords[i] - 450, f'({x_coords[i]}, {y_coords[i]})', 
             fontsize=11, ha='center', color=point_colors[i])

# 5. Define and add unabbreviated formula text to the plot
# Segment 1
c1 = compact_coeffs[0]
d1 = direct_coeffs[0]
formula1_text = (
    fr"$\bf{{Formula\ for\ 1920 \leq x \leq 1987}}$ (Curve: Blue)" + "\n\n"
    r"$\bf{Compact\ Form:}$" + "\n"
    fr"$y = {c1['a']:.6f}(x-1920)^3 {c1['b']:+.4f}(x-1920)^2$" + "\n"
    fr"${c1['c']:+.2f}(x-1920) {c1['d']:+.2f}$" + "\n\n"
    r"$\bf{Direct\ Formula:}$" + "\n"
    fr"$y = {d1['A']:.8f}x^3 {d1['B']:+.4f}x^2 {d1['C']:+.2f}x {d1['D']:+.2f}$"
)

# Segment 2
c2 = compact_coeffs[1]
d2 = direct_coeffs[1]
formula2_text = (
    fr"$\bf{{Formula\ for\ 1987 \leq x \leq 3128}}$ (Curve: Orange)" + "\n\n"
    r"$\bf{Compact\ Form:}$" + "\n"
    fr"$y = {c2['a']:.6f}(x-1987)^3 {c2['b']:+.4f}(x-1987)^2$" + "\n"
    fr"${c2['c']:+.2f}(x-1987) {c2['d']:+.2f}$" + "\n\n"
    r"$\bf{Direct\ Formula:}$" + "\n"
    fr"$y = {d2['A']:.8f}x^3 {d2['B']:+.4f}x^2 {d2['C']:+.2f}x {d2['D']:+.2f}$"
)

# Adjust y-position of the second text box to move it up
ax.text(0.05, 0.95, formula1_text, transform=ax.transAxes, fontsize=11,
        verticalalignment='top', bbox=dict(boxstyle='round,pad=0.5', fc='lightblue', alpha=0.5))
ax.text(0.05, 0.75, formula2_text, transform=ax.transAxes, fontsize=11, 
        verticalalignment='top', bbox=dict(boxstyle='round,pad=0.5', fc='orange', alpha=0.4))


# 6. Create a custom legend
handles = [
    Line2D([0], [0], color=curve_colors[0], lw=2, label='Curve Segment 1'),
    Line2D([0], [0], color=curve_colors[1], lw=2, label='Curve Segment 2'),
    Line2D([0], [0], marker='o', color='white', label=''), 
    Line2D([0], [0], marker='o', color=point_colors[0], label=f'Point 1 ({x_coords[0]}, {y_coords[0]})', linestyle='None'),
    Line2D([0], [0], marker='o', color=point_colors[1], label=f'Point 2 ({x_coords[1]}, {y_coords[1]})', linestyle='None'),
    Line2D([0], [0], marker='o', color=point_colors[2], label=f'Point 3 ({x_coords[2]}, {y_coords[2]})', linestyle='None')
]
ax.legend(handles=handles, title="Elements", fontsize=11)

# 7. Final customizations and save
ax.set_title('Green-Eye Sensor Device Soil EC Sensor', fontsize=16)
ax.set_xlabel('X Coordinate', fontsize=12)
ax.set_ylabel('Y Coordinate', fontsize=12)
ax.grid(True)
plt.tight_layout()
plt.savefig('green-eye_soil_ec_graph.png')

print("Graph has been saved.")
