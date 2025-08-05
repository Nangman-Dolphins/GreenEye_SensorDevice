import numpy as np

print("\n======= 3차 계수 산출기 ========\n")

# Data points.
x_coords = np.array([1920, 1987, 1948, 3128])
y_coords = np.array([84, 1413, 233, 12880])

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

print("\n" + "="*50)
print("   실시간 값 변환기 (input 'q' for quit)")
print("="*50)

while True:
    user_input = input("ADC_raw: ")

    if user_input.lower() == 'q':
        print("Bye\n")
        break
    
    try:
        input_value = float(user_input)

        result = convert_sensor_value_from_local_coeffs(input_value, coefficients)

        print(f"-> result: {result:.2f}\n")

    except ValueError:
        print("ERR: Wrong Input.\n")
