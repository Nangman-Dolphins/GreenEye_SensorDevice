import numpy as np

print("--- 3차 계수 산출기 ---")

# Data points.
x_coords = np.array([1920, 1987, 3128, 1948])
y_coords = np.array([84, 1413, 12880, 233])

coefficients = np.polyfit(x_coords, y_coords, 3)

print("--- 산출된 로컬 계수 ---")
a, b, c, d = coefficients
print(f"COFF_A = {a:.20f}")
print(f"COFF_B = {b:.20f}")
print(f"COFF_C = {c:.20f}")
print(f"COFF_D = {d:.20f}")
print("-" * 35 + "\n")

def convert_sensor_value_from_local_coeffs(adc_raw_value, coeffs):
  polynomial_function = np.poly1d(coeffs)
  return polynomial_function(adc_raw_value)


print("--- 로컬 계수 검증 결과 ---")
for x, y_expected in zip(x_coords, y_coords):
    y_calculated = convert_sensor_value_from_local_coeffs(x, coefficients)
    print(f"입력: {x}, 기대값: {y_expected}, 계산값: {y_calculated:.2f}")
