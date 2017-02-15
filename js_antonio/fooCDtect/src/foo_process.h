// структура взята с: http://www.codenet.ru/progr/formt/rawsam.php
// с небольшой модификацией
typedef struct {
	char	id_riff[4];
	long	len_riff;

	char	id_chuck[4];
	char	fmt[4];
	long	len_chuck;

	short	type;
	short 	channels;
	long	freq;
	long	bytes;
	short 	align;
	short 	bits;

	char	id_data[4];
	long	len_data;
} TitleWave;

struct tagCMDL{
	wchar_t		imagepath[MAX_PATH]; // имя экзешки
	wchar_t		outputfilename[MAX_PATH];

	wchar_t		temppath[MAX_PATH];

	int			threads; // количество потоков
	int			mode; // строгость проверки

	bool		isGUI; // 1 - gui, 2 - процесс
	bool		autodel;

	int			lformat;
	int			lencoding;
	int			priority;
};

int GoProcessing(const tagCMDL &);



