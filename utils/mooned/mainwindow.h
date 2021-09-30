
#pragma once

class MdiChild;

class MainWindow final : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();

	bool openFile( const QString &fileName );

protected:
	void closeEvent( QCloseEvent *event ) override;

private slots:
	void newFile();
	void open();
	void save();
	void saveAs();
	void updateRecentFileActions();
	void openRecentFile();
#ifndef QT_NO_CLIPBOARD
	void cut();
	void copy();
	void paste();
#endif
	void about();
	void updateMenus();
	void updateWindowMenu();
	MdiChild *createMdiChild();
	void switchLayoutDirection();

	void onToolSelect();

private:
	enum { MaxRecentFiles = 5 };

	void createActions();
	void createStatusBar();
	void readSettings();
	void writeSettings();
	bool loadFile( const QString &fileName );
	static bool hasRecentFiles();
	void prependToRecentFiles( const QString &fileName );
	void setRecentFilesVisible( bool visible );
	MdiChild *activeMdiChild() const;
	QMdiSubWindow *findMdiChild( const QString &fileName ) const;

	QMdiArea *	mdiArea;

	QMenu *		windowMenu;
	QAction *	newAct;
	QAction *	saveAct;
	QAction *	saveAsAct;
	QAction *	recentFileActs[MaxRecentFiles];
	QAction *	recentFileSeparator;
	QAction *	recentFileSubMenuAct;
#ifndef QT_NO_CLIPBOARD
	QAction *	cutAct;
	QAction *	copyAct;
	QAction *	pasteAct;
#endif
	QAction *	closeAct;
	QAction *	closeAllAct;
	QAction *	tileAct;
	QAction *	cascadeAct;
	QAction *	nextAct;
	QAction *	previousAct;
	QAction *	windowMenuSeparatorAct;

	QAction *	toolSelectAct;

	QAction *	toolEntityAct;
	QAction *	toolBrushAct;

	QAction *	toolMaterialAct;
	QAction *	toolApplyAct;

	QAction *	toolClipAct;
	QAction *	toolVertexAct;
};

extern MainWindow *g_pMainWindow;
