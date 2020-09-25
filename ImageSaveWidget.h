
#ifndef _IMAGE_SAVE_DIALOG_H_
#define _IMAGE_SAVE_DIALOG_H_

#include "Util.h"
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QProgressDialog>
#include <QtCore/QString>
#include <QtWidgets/QFileDialog>
#include "imagesavethread.h"

#define IMAGE_SAVE_THREADS 8

class ImageSaveWidget : public QDialog
{
	Q_OBJECT
public:
	static string OBJECT_NAME;
	explicit ImageSaveWidget(DisplayWindowFlag windowFlag, QWidget* parent=0);

	ImageBuffer* Allocate(ImageSize, DATATYPE);
	void SaveImages();
	static void SaveOneImage(const string& filename, void* image_data, DATATYPE dataType, int width, int height);
	inline int Get_ImageNum(){ return ImageNum; }

signals:
	void StartSaveImageSignal(int);

protected:
	void CreateLayout();

protected slots:
	void OnImageNumChanged();
	void OnImageFolderButton();
	void OnOkButton();
	void OnCancelButton();
	void OnImageFormatChanged();
	void OnImageItemSaved();

private:
	QLineEdit* fileFolderEdit;
	QLineEdit* fileNamePrefixEdit;
	QLineEdit* imageNumEdit;
	QRadioButton* tiffFormatButton;
	QRadioButton* rawFormatButton;
	QPushButton* imageFolderButton;
	QPushButton* okButton;
	QPushButton* cancelButton;
	QProgressDialog *progressDialog;

	DisplayWindowFlag window;
	QString prefix, imageFormat, imageFolder;
	int ImageNum;

	int savedNum;
	ImageSaveThread *threads[IMAGE_SAVE_THREADS];

	void finishWorks();
};

#endif // _IMAGE_SAVE_DIALOG_H_