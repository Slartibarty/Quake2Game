
#pragma once

#include "mapview2d.h"
#include "mapview3d.h"

#include <vector>

struct linePoint_t
{
	float x, y, z;
	uint32 color;
};

struct lineSegment_t
{
	linePoint_t p1;
	linePoint_t p2;
};

class MdiChild : public QWidget
{
	Q_OBJECT

private:
	QGridLayout gridLayout;
	MapView3D view1;
	MapView2D view2;
	MapView2D view3;
	MapView2D view4;

	QString m_curFile;
	bool m_isUntitled;

	std::vector<lineSegment_t> m_lineSegments;

public:
	MdiChild();

	void NewFile();
	bool LoadFile( const QString &fileName );
	bool Save();
	bool SaveAs();
	bool SaveFile( const QString &fileName );
	QString UserFriendlyCurrentFile();
	QString CurrentFile() { return m_curFile; }

protected:
	void closeEvent( QCloseEvent *event ) override;

private slots:
	void documentWasModified();

private:
	bool MaybeSave();
	void SetCurrentFile( const QString &fileName );
	QString StrippedName( const QString &fullFileName );

};
