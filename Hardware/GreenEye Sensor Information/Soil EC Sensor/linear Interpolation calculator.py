import numpy as np

print("\n======= 선형 보간 값 변환기 ========\n") 

x_points = np.array([1920, 1948, 1987, 3128])
y_points = np.array([84, 233, 1413, 12880])

def convert_sensor_value_linear(adc_raw_value):
  return np.interp(adc_raw_value, x_points, y_points)


print("--- 기준 데이터 검증 결과 ---")
x_coords_original = np.array([1920, 1948, 1987, 3128])
y_coords_original = np.array([84, 233, 1413, 12880])

for x, y_expected in zip(x_coords_original, y_coords_original):
    y_calculated = convert_sensor_value_linear(x)
    print(f"입력: {x}, 기대값: {y_expected}, 계산값: {y_calculated:.2f}")

print("\n" + "="*50)
print("    실시간 값 변환기 (input 'q' for quit)")
print("="*50)

while True:
    user_input = input("ADC_raw: ")

    if user_input.lower() == 'q':
        print("Bye\n")
        break
    
    try:
        input_value = float(user_input)


        result = convert_sensor_value_linear(input_value)

        print(f"-> result: {result:.2f}\n")

    except ValueError:
        print("ERR: Wrong Input.\n")
