
#include "ImageSaveWidget.h"
#include "DevicePackage.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMessageBox>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#define MAX_IMAGE_NUM 10000
string ImageSaveWidget::OBJECT_NAME = "ImageSaveWidget";

ImageSaveWidget::ImageSaveWidget(DisplayWindowFlag windowFlag, QWidget *parent):QDialog(parent)
{
	window = windowFlag;
    imageFormat = "tiff";
	imageFolder = "E:\\";
	ImageNum = 1;
	savedNum = 0;
	for (int i = 0; i < IMAGE_SAVE_THREADS; i++) threads[i] = NULL;
    CreateLayout();
}

void ImageSaveWidget::CreateLayout()
{    
	fileFolderEdit = new QLineEdit;
	fileNamePrefixEdit = new QLineEdit;
	imageNumEdit = new QLineEdit;
	tiffFormatButton = new QRadioButton("Tiff");
	rawFormatButton = new QRadioButton("Raw");
	imageFolderButton = new QPushButton("...");
	okButton = new QPushButton("OK");
	cancelButton = new QPushButton("Cancel");

	imageNumEdit->setText( QString::number(ImageNum) );
	tiffFormatButton->setChecked(true);
	rawFormatButton->setChecked(false);
	imageFolderButton->setMaximumWidth(35);
	imageFolderButton->setMinimumWidth(25);
	fileFolderEdit->setReadOnly(true);
		
	QObject::connect( imageNumEdit, SIGNAL( editingFinished() ), this, SLOT( OnImageNumChanged() ));
	QObject::connect( imageFolderButton, SIGNAL( clicked() ), this, SLOT( OnImageFolderButton() ));
	QObject::connect( okButton, SIGNAL( clicked() ), this, SLOT( OnOkButton() ));
	QObject::connect( cancelButton, SIGNAL( clicked() ), this, SLOT( OnCancelButton() ));
	
    QHBoxLayout *fileFolderLayout = new QHBoxLayout;
	fileFolderLayout->addWidget( new QLabel("Image Folder") );
	fileFolderLayout->addWidget( fileFolderEdit );
	fileFolderLayout->addWidget( imageFolderButton );
	fileFolderLayout->insertSpacing(1, 17);

	QHBoxLayout* imageNamePrefixLayout = new QHBoxLayout;
	imageNamePrefixLayout->addWidget(new QLabel("Filename Prefix") );
	imageNamePrefixLayout->addWidget(fileNamePrefixEdit);

	QHBoxLayout *imageSettingLayout = new QHBoxLayout;
	imageSettingLayout->addWidget( new QLabel("Format") );
	imageSettingLayout->addWidget( tiffFormatButton );
	imageSettingLayout->addWidget( rawFormatButton );
	imageSettingLayout->addWidget( new QLabel("Image Num") );
	imageSettingLayout->addWidget( imageNumEdit );
	
	QHBoxLayout* buttonsLayout = new QHBoxLayout;
	buttonsLayout->addWidget( okButton );
	buttonsLayout->addWidget( cancelButton );
	buttonsLayout->setSpacing(20);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(fileFolderLayout);
	mainLayout->addLayout(imageNamePrefixLayout);
	mainLayout->addLayout(imageSettingLayout);
	mainLayout->addLayout( buttonsLayout );
	
	progressDialog = new QProgressDialog();
	if (window == (int)HAMAMATSU_WINDOW){
		progressDialog->setWindowTitle("Saving Hamamatsu Images");
	}
	else if (window == (int)ANDOR_WINDOW){
		progressDialog->setWindowTitle("Saving Andor Images");
	}
	else if (window == (int)IO_WINDOW){
		progressDialog->setWindowTitle("Saving IO Images");
	}
	connect(progressDialog, SIGNAL(canceled()), this, SLOT(OnCancelButton()));
	progressDialog->close();
    
	 this->setLayout(mainLayout);
	 this->setFixedWidth(300);
}

void ImageSaveWidget::OnImageNumChanged()
{
	ImageNum = (imageNumEdit->text()).toInt();
	if (ImageNum > MAX_IMAGE_NUM){
		QMessageBox::critical(this, "Error", "Now maximum image save capacity is "+QString::number(MAX_IMAGE_NUM) );
	}
	if (ImageNum <= 0){
		QMessageBox::critical(this, "Error", "Invalid image num" );
	}
}

void ImageSaveWidget::OnImageFormatChanged()
{
	if (tiffFormatButton->isChecked()){
		imageFormat = "tiff";
	}
	else if (rawFormatButton->isChecked()){
		imageFormat = "raw";
	}
}

void ImageSaveWidget::OnImageFolderButton()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Save Images"), imageFolder,  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	fileFolderEdit->setText(dir);
	imageFolder = dir;
}

void ImageSaveWidget::OnOkButton()
{
	if (imageFolder.isEmpty()){
		QMessageBox::warning(this, "Warning", "Please select image folder");
		return;
	}

	if (window == (int)HAMAMATSU_WINDOW){
		if (HamamatsuImageBuffers != NULL){
			delete HamamatsuImageBuffers;
			HamamatsuImageBuffers = NULL;
		}
		HamamatsuStartSaveImage = true;
	}
	/*else if (window == (int)ANDOR_WINDOW){
		if (AndorImageBuffers != NULL){
			delete AndorImageBuffers;
			AndorImageBuffers = NULL;
		}
		AndorStartSaveImage = true;
	}
	else if (window == (int)IO_WINDOW){
		if (IOImageBuffers != NULL){
			delete IOImageBuffers;
			IOImageBuffers = NULL;
		}
		IOStartSaveImage = true;
	}*/
	emit StartSaveImageSignal((int)window); //send signal to related camera
	this->hide();
}

void ImageSaveWidget::OnCancelButton()
{
	finishWorks();
	this->hide();
}

void ImageSaveWidget::finishWorks() {
	for (int i = 0; i < IMAGE_SAVE_THREADS; i++) {
		if (threads[i]) {
			threads[i]->stop();
		}
	}
	bool allStopped;
	while (true) {
		allStopped = true;
		for (int i = 0; i < IMAGE_SAVE_THREADS; i++) {
			if (threads[i]) {
				if (threads[i]->isStoped()) {
					delete threads[i];
					threads[i] = NULL;
				} else {
					allStopped = false;
					break;
				}
			}
		}
		if (allStopped) break;
	}
	progressDialog->close();
}

void ImageSaveWidget::OnImageItemSaved()
{
	this->savedNum++;
	if (!progressDialog->isHidden()) progressDialog->setValue(this->savedNum);
	if (this->savedNum == ImageNum) {
		//cout << "saveNum == ImageNum" << this->window << this->savedNum << ImageNum;
		finishWorks();
		if (window == (int)HAMAMATSU_WINDOW){
			QMessageBox::information(this, "Information", "Finish to save Hamamatsu images");
			HamamatsuImageBuffers = NULL;
		}
		/*else if (window == (int)ANDOR_WINDOW){
			QMessageBox::information(this, "Information", "Finish to save Andor images");
			AndorImageBuffers = NULL;
		}
		else if (window == (int)IO_WINDOW){
			QMessageBox::information(this, "Information", "Finish to save IO images");
			IOImageBuffers = NULL;
		}*/
	}
}

ImageBuffer* ImageSaveWidget::Allocate(ImageSize imageSize, DATATYPE type)
{
	OnImageNumChanged();
	ImageBuffer* buffers = new ImageBuffer[ImageNum];
	for (int i=0; i<ImageNum; ++i){
		buffers[i].image_width = imageSize.width;
		buffers[i].image_height = imageSize.height;
		buffers[i].data_type = type;
		if (type == USHORT_TYPE){
			buffers[i].image_data = (void*)new ushort[imageSize.width*imageSize.height];
		}
		else if (type == UCHAR_TYPE){
			buffers[i].image_data = (void*)new uchar[imageSize.width*imageSize.height];
		}
	}
	return buffers;
}

//void ImageSaveWidget::SaveImages(ImageBuffer* buffers)
//{
//	if (buffers == NULL){ return; }
//	prefix = fileNamePrefixEdit->text();
//	imageFormat = "tiff";
//
//	QMessageBox::information(this, "Information", "Start saving images to disk");
//	// Save each image in buffers to disk
//	for (int i=0; i<ImageNum; ++i){
//		QString filename = imageFolder+"\\"+prefix+"_"+QString::number(i+1)+"."+imageFormat;
//
//		if (buffers[i].data_type == USHORT_TYPE){
//			ushort* image_data = (ushort*)buffers[i].image_data;
//			cv::Mat image(buffers[i].image_height, buffers[i].image_width, CV_16UC1, image_data);
//			imwrite(filename.toStdString(), image);
//		}
//		else if (buffers[i].data_type == UCHAR_TYPE){
//			uchar* image_data = (uchar*)buffers[i].image_data;
//			cv::Mat image(buffers[i].image_height, buffers[i].image_width, CV_8UC1, image_data);
//			imwrite(filename.toStdString(), image);
//		}
//
//		delete buffers[i].image_data;
//		buffers[i].image_data = NULL;
//	}
//	delete buffers;
//	QMessageBox::information(this, "Information", "Save image complete");
//}

void ImageSaveWidget::SaveImages() 
{
	finishWorks();

	this->savedNum = 0;
	progressDialog->setRange(0, ImageNum);
	progressDialog->setValue(0);
	progressDialog->open();

	ImageBuffer* buffers = NULL;
	if (window == (int)HAMAMATSU_WINDOW){
		buffers = HamamatsuImageBuffers;
	}
	/*else if (window == (int)ANDOR_WINDOW){
		buffers = AndorImageBuffers;
	}
	else if (window == (int)IO_WINDOW){
		buffers = IOImageBuffers;
	}*/

	if (buffers == NULL){ return; }
	prefix = fileNamePrefixEdit->text();
	imageFormat = "tiff";

	unsigned int sliceCountQuot = ImageNum / IMAGE_SAVE_THREADS;
	unsigned int sliceCountRem = ImageNum - sliceCountQuot * IMAGE_SAVE_THREADS;
	unsigned int sliceCount;
	unsigned int sliceOffset = 0;
	for (int i = 0; i < IMAGE_SAVE_THREADS; i++) {
		if (i < sliceCountRem) sliceCount = sliceCountQuot + 1;
		else sliceCount = sliceCountQuot;
		threads[i] = new ImageSaveThread(buffers + sliceOffset, sliceCount, imageFolder, prefix, imageFormat, this);
		connect(threads[i], SIGNAL(onImageSaved(ImageBuffer *)), this, SLOT(OnImageItemSaved()));
		sliceOffset += sliceCount;
	}
	for (int i = 0; i < IMAGE_SAVE_THREADS; i++) threads[i]->start();
}

 void ImageSaveWidget::SaveOneImage(const string& filename, void* image_data, DATATYPE data_type, int width, int height)
 {
	 if (data_type == USHORT_TYPE){
		ushort* data = (ushort*)image_data;
		cv::Mat image(height, width, CV_16UC1, data);
		imwrite(filename, image);
	}
	else if (data_type == UCHAR_TYPE){
		uchar* data = (uchar*)image_data;
		cv::Mat image(height, width, CV_8UC1, data);
		imwrite(filename, image);
	}
 }