#define wndclassnameW L"FooCDtect2_f11e0af6-759f-434b-8121-359b0e306a6d"

// ������� ������ ��������������. G ������ � ������ ����� ������ �
// ���������� ����� ������
#define MES_ONNEWTHREAD		WM_APP+1

// ��� ��������� CopyData. wParam ����� �����, � ��������� dwData:
// 1 - ���� ��� �����, 2 - ���� stdout. lpData - ������
//#define FILENAME			L'1'

// � wParam ������� ����� ������, � � lParam ��������� �� 0 �� 100
#define MES_PROGRESS		WM_APP+2

// � wParam ������� ����� ������, � � lParam ���������
// ���������: 
//		hi word - 0..2 (0 - CD, 1 - MPEG, 2 - UNKNOWN)
//		lo word - ����������� �� 0 �� 100
//#define MES_RESULT			WM_APP+3

// ��� ��������� CopyData. ��������� � ������ ���������� ������.
// ���� ������. ��������� �� stdout.
//#define RES_OUTPUT			L'2'

// � wParam ������� ����� ������. ���������� ���. ��������� �������� �����,
// ���������� �������
//#define MES_ENDPROCESS		WM_APP+4

// � wParam ������� ����� ������. � lParam ������������ � �� ( h.m.s.mmm )
#define MES_SENDLENGTH		WM_APP+5

#define MES_GETPRIORITY		WM_APP+7

#define MES_SENDPROCID1		WM_APP+8
#define MES_SENDPROCID2		WM_APP+9
