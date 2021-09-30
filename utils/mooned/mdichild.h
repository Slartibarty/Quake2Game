
#pragma once

#include "mapview2d.h"
#include "mapview3d.h"

class MdiChild : public QWidget
{
	Q_OBJECT

private:

	QGridLayout gridLayout;
	MapView3D view1;
	MapView2D view2;
	MapView2D view3;
	MapView2D view4;

public:
	MdiChild();

	void newFile();
	bool loadFile( const QString &fileName );
	bool save();
	bool saveAs();
	bool saveFile( const QString &fileName );
	QString userFriendlyCurrentFile();
	QString currentFile() { return curFile; }

protected:
	void closeEvent( QCloseEvent *event ) override;

private slots:
	void documentWasModified();

private:
	bool maybeSave();
	void setCurrentFile( const QString &fileName );
	QString strippedName( const QString &fullFileName );

	QString curFile;
	bool isUntitled;
};
