
#pragma once

class MdiChild : public QWidget
{
	Q_OBJECT

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
