#define wndclassnameW L"FooCDtect2_f11e0af6-759f-434b-8121-359b0e306a6d"

// попытка начать взаимодействие. G создаёт в списке новую запись и
// возвращает номер записи
#define MES_ONNEWTHREAD		WM_APP+1

// Для сообщения CopyData. wParam номер итема, в структуре dwData:
// 1 - если имя файла, 2 - если stdout. lpData - данные
//#define FILENAME			L'1'

// В wParam передаём номер записи, а в lParam положение от 0 до 100
#define MES_PROGRESS		WM_APP+2

// В wParam передаём номер записи, в в lParam результат
// результат: 
//		hi word - 0..2 (0 - CD, 1 - MPEG, 2 - UNKNOWN)
//		lo word - вероятность от 0 до 100
//#define MES_RESULT			WM_APP+3

// Для сообщения CopyData. Прилепить в начало посылаемой строки.
// Один символ. Результат из stdout.
//#define RES_OUTPUT			L'2'

// В wParam передаём номер записи. Параметров нет. Окончание проверки файла,
// уменьшение очереди
//#define MES_ENDPROCESS		WM_APP+4

// В wParam передаём номер записи. В lParam длительность в мс ( h.m.s.mmm )
#define MES_SENDLENGTH		WM_APP+5

#define MES_GETPRIORITY		WM_APP+7

#define MES_SENDPROCID1		WM_APP+8
#define MES_SENDPROCID2		WM_APP+9
