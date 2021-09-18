
#include "mooned_local.h"

#include "mapview2d.h"

MapView2D::MapView2D()
{
	setAttribute( Qt::WA_DeleteOnClose );
	isUntitled = true;
}

void MapView2D::newFile()
{
	static int sequenceNumber = 1;

	isUntitled = true;
	curFile = tr( "document%1.txt" ).arg( sequenceNumber++ );
	setWindowTitle( curFile + "[*]" );

	connect( document(), &QTextDocument::contentsChanged, this, &MapView2D::documentWasModified );
}

bool MapView2D::loadFile( const QString &fileName )
{
	QFile file( fileName );
	if ( !file.open( QFile::ReadOnly | QFile::Text ) )
	{
		QMessageBox::warning( this, tr( "MDI" ),
			tr( "Cannot read file %1:\n%2." )
			.arg( fileName )
			.arg( file.errorString() ) );
		return false;
	}

	QTextStream in( &file );
	QGuiApplication::setOverrideCursor( Qt::WaitCursor );
	setPlainText( in.readAll() );
	QGuiApplication::restoreOverrideCursor();

	setCurrentFile( fileName );

	connect( document(), &QTextDocument::contentsChanged,
		this, &MapView2D::documentWasModified );

	return true;
}

bool MapView2D::save()
{
	if ( isUntitled )
	{
		return saveAs();
	}
	else
	{
		return saveFile( curFile );
	}
}

bool MapView2D::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save As" ), curFile );
	if ( fileName.isEmpty() )
	{
		return false;
	}

	return saveFile( fileName );
}

bool MapView2D::saveFile( const QString &fileName )
{
	QString errorMessage;

	QGuiApplication::setOverrideCursor( Qt::WaitCursor );
	QSaveFile file( fileName );
	if ( file.open( QFile::WriteOnly | QFile::Text ) )
	{
		QTextStream out( &file );
		out << toPlainText();
		if ( !file.commit() )
		{
			errorMessage = tr( "Cannot write file %1:\n%2." ).arg( QDir::toNativeSeparators( fileName ), file.errorString() );
		}
	}
	else
	{
		errorMessage = tr( "Cannot open file %1 for writing:\n%2." ).arg( QDir::toNativeSeparators( fileName ), file.errorString() );
	}
	QGuiApplication::restoreOverrideCursor();

	if ( !errorMessage.isEmpty() )
	{
		QMessageBox::warning( this, tr( "MDI" ), errorMessage );
		return false;
	}

	setCurrentFile( fileName );
	return true;
}

QString MapView2D::userFriendlyCurrentFile()
{
	return strippedName( curFile );
}

void MapView2D::closeEvent( QCloseEvent *event )
{
	if ( maybeSave() )
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void MapView2D::documentWasModified()
{
	setWindowModified( document()->isModified() );
}

bool MapView2D::maybeSave()
{
	if ( !document()->isModified() )
	{
		return true;
	}

	const QMessageBox::StandardButton ret
		= QMessageBox::warning( this, tr( "MDI" ),
			tr( "'%1' has been modified.\n"
				"Do you want to save your changes?" )
			.arg( userFriendlyCurrentFile() ),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel );

	switch ( ret )
	{
	case QMessageBox::Save:
		return save();

	case QMessageBox::Cancel:
		return false;

	default:
		break;
	}
	return true;
}

void MapView2D::setCurrentFile( const QString &fileName )
{
	curFile = QFileInfo( fileName ).canonicalFilePath();
	isUntitled = false;
	document()->setModified( false );
	setWindowModified( false );
	setWindowTitle( userFriendlyCurrentFile() + "[*]" );
}

QString MapView2D::strippedName( const QString &fullFileName )
{
	return QFileInfo( fullFileName ).fileName();
}
