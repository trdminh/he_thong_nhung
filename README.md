## Hệ thống nhúng

### Bài 1:

Nạp chương trình vào trong kit stm32f103c8t6 bằng phần mềm keilC.

### Bài 2:
- Yêu cầu 1: Thực hiện nhấp nháy led với chu kỳ 1000ms:
    + Tạo hàm delay sử dụng TIM2 của stm.
    + Cấu hình GPIO_Pin_13 của GPIOC ở chế độ output push pull dùng làm chân điều khiển led.
    + Hàm GPIO_SetBits và GPIO_ResetBits dùng để thay đổi trạng thái chân GPIO_Pin_13. 

- Yêu cầu 2: Sử dụng nút nhấn để điều khiển led.
    + Nối nút nhấn với kit stm32f103c8t6 theo kiểu pull up và cấu hình cho chân GPIO của nút nhấn.
    ![Sơ đồ nút nhấn pull-up](img/button.jpg)
    + Sử dụng GPIO_Pin_13 của GPIOC để làm led được điều khiển.

### Bài 3:

Ứng dụng Ngắt ngoài (External Interrupt) và Timer để điều khiển LED:

- Ngắt ngoài (EXTI):
    + Cấu hình GPIO_Pin_5 của GPIOA ở chế độ input pull-up để kết nối với nút nhấn.
    + Cấu hình ngắt ngoài EXTI5 với chế độ kích hoạt ngắt khi có cạnh xuống (Falling edge).
    + Cấu hình NVIC để xử lý ngắt ngoài EXTI9_5_IRQn với ưu tiên cao nhất.
    + Khi nút nhấn được kích hoạt, trạng thái LED PC13 sẽ được đảo ngược (toggle).

- Timer (TIM2):
    + Cấu hình Timer2 với tần số 10kHz (72MHz/7200), chu kỳ 10000 tick (tạo ngắt mỗi 1 giây).
    + Kích hoạt ngắt Update của Timer2 và cấu hình NVIC với mức ưu tiên thấp hơn ngắt ngoài.
    + Khi Timer2 đếm đủ chu kỳ, LED PA1 sẽ tự động đảo trạng thái (toggle).

- Chức năng:
    + LED PC13 được điều khiển thông qua nút nhấn kết nối với PA5.
    + LED PA1 tự động nhấp nháy với tần số 0.5Hz (chu kỳ 2 giây).

### Bài 4:

Ứng dụng Timer để điều khiển LED nhấp nháy:

- Cấu hình GPIO:
    + Cấu hình GPIO_Pin_13 của GPIOC ở chế độ output push-pull để điều khiển LED.
    + Ban đầu LED được tắt (GPIO_SetBits).

- Timer (TIM2):
    + Cấu hình Timer2 với tần số 10kHz (72MHz/7200), chu kỳ 5000 tick (tạo ngắt mỗi 0.5 giây).
    + Kích hoạt ngắt Update của Timer2 và cấu hình NVIC.
    + Khi Timer2 đếm đủ chu kỳ, LED PC13 sẽ tự động đảo trạng thái (toggle).

- Chức năng:
    + LED PC13 tự động nhấp nháy với tần số 1Hz (chu kỳ 1 giây).
    + Sử dụng phép XOR bit để đảo trạng thái của LED (GPIOC->ODR ^= GPIO_Pin_13).