#include <stdio.h> // 1. 헤더 포함
#include "UART0.h" // UART 라이브러리 (UART0_transmit, UART0_receive 포함)

// 2. 스트림 객체 생성
// 출력 스트림: UART0_transmit 함수 연결, 쓰기 모드(_FDEV_SETUP_WRITE)
FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);
// 입력 스트림: UART0_receive 함수 연결, 읽기 모드(_FDEV_SETUP_READ)
FILE INPUT = FDEV_SETUP_STREAM(NULL, UART0_receive, _FDEV_SETUP_READ);

int main(void)
{
	// 3. 표준 입출력 연결
	stdout = &OUTPUT;
	stdin = &INPUT;

	UART0_init(); // 하드웨어 초기화

	int counter = 100;
	char buffer[20];

	printf("Start System\r\n"); // 사용 예시

	while (1)
	{
		printf("Input Command: ");
		scanf("%s", buffer); // UART로 문자열 입력 받기
		printf("Received: %s, Counter: %d\r\n", buffer, counter++);
	}
	return 0;
}