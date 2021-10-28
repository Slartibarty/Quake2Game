
#include "mooned_local.h"

#include "mdichild.h"

MdiChild::MdiChild()
	: gridLayout( this )
	, view1( this ), view2( this )
	, view3( this ), view4( this )
{
	//gridLayout.setSpacing( 0 );
	gridLayout.setContentsMargins( QMargins( 0, 0, 0, 0 ) );
	gridLayout.addWidget( &view1, 1, 1 );
	gridLayout.addWidget( &view2, 1, 2 );
	gridLayout.addWidget( &view3, 2, 1 );
	gridLayout.addWidget( &view4, 2, 2 );

	setAttribute( Qt::WA_DeleteOnClose );
	m_isUntitled = true;
}

void MdiChild::NewFile()
{
	static int sequenceNumber = 1;

	m_isUntitled = true;
	m_curFile = tr( "document%1.txt" ).arg( sequenceNumber++ );
	setWindowTitle( m_curFile + "[*]" );

	// LEGACY
	//connect( document(), &QTextDocument::contentsChanged, this, &MdiChild::documentWasModified );
}

bool MdiChild::LoadFile( const QString &fileName )
{
	QFile file( fileName );
	if ( !file.open( QFile::ReadOnly | QFile::Text ) )
	{
		QMessageBox::warning( this, QApplication::applicationDisplayName(),
			tr( "Cannot read file %1:\n%2." )
			.arg( fileName )
			.arg( file.errorString() ) );
		return false;
	}

	QTextStream in( &file );
	QGuiApplication::setOverrideCursor( Qt::WaitCursor );
	// LEGACY
	//setPlainText( in.readAll() );
	QGuiApplication::restoreOverrideCursor();

	SetCurrentFile( fileName );

	// LEGACY
	//connect( document(), &QTextDocument::contentsChanged, this, &MdiChild::documentWasModified );

	return true;
}

bool MdiChild::Save()
{
	if ( m_isUntitled )
	{
		return SaveAs();
	}
	else
	{
		return SaveFile( m_curFile );
	}
}

bool MdiChild::SaveAs()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save As" ), m_curFile );
	if ( fileName.isEmpty() )
	{
		return false;
	}

	return SaveFile( fileName );
}

bool MdiChild::SaveFile( const QString &fileName )
{
	QString errorMessage;

	QGuiApplication::setOverrideCursor( Qt::WaitCursor );
	QSaveFile file( fileName );
	if ( file.open( QFile::WriteOnly | QFile::Text ) )
	{
		// LEGACY
		//QTextStream out( &file );
		//out << toPlainText();
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
		QMessageBox::warning( this, QApplication::applicationDisplayName(), errorMessage );
		return false;
	}

	SetCurrentFile( fileName );
	return true;
}

QString MdiChild::UserFriendlyCurrentFile()
{
	return StrippedName( m_curFile );
}

void MdiChild::closeEvent( QCloseEvent *event )
{
	if ( MaybeSave() )
	{
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void MdiChild::documentWasModified()
{
	// LEGACY
	//setWindowModified( document()->isModified() );
}

bool MdiChild::MaybeSave()
{
	// LEGACY
	//if ( !document()->isModified() )
	//{
	//	return true;
	//}

	const QMessageBox::StandardButton ret
		= QMessageBox::warning( this, QApplication::applicationDisplayName(),
			tr( "'%1' has been modified.\n"
				"Do you want to save your changes?" )
			.arg( UserFriendlyCurrentFile() ),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel );

	switch ( ret )
	{
	case QMessageBox::Save:
		return Save();

	case QMessageBox::Cancel:
		return false;

	default:
		break;
	}
	return true;
}

void MdiChild::SetCurrentFile( const QString &fileName )
{
	m_curFile = QFileInfo( fileName ).canonicalFilePath();
	m_isUntitled = false;
	// LEGACY
	//document()->setModified( false );
	setWindowModified( false );
	setWindowTitle( UserFriendlyCurrentFile() + "[*]" );
}

QString MdiChild::StrippedName( const QString &fullFileName )
{
	return QFileInfo( fullFileName ).fileName();
}
