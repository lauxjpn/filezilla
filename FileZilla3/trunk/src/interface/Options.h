#ifndef FILEZILLA_INTERFACE_OPTIONS_HEADER
#define FILEZILLA_INTERFACE_OPTIONS_HEADER

#include "../include/local_path.h"
#include "../include/engine_options.h"

#include <libfilezilla/mutex.hpp>

#include <wx/timer.h>

enum interfaceOptions
{
	// Default/internal options
	OPTION_DEFAULT_SETTINGSDIR, // guaranteed to be (back)slash-terminated
	OPTION_DEFAULT_KIOSKMODE,
	OPTION_DEFAULT_DISABLEUPDATECHECK,
	OPTION_DEFAULT_CACHE_DIR,

	OPTION_NUMTRANSFERS,
	OPTION_ASCIIBINARY,
	OPTION_ASCIIFILES,
	OPTION_ASCIINOEXT,
	OPTION_ASCIIDOTFILE,
	OPTION_LANGUAGE,
	OPTION_CONCURRENTDOWNLOADLIMIT,
	OPTION_CONCURRENTUPLOADLIMIT,
	OPTION_UPDATECHECK,
	OPTION_UPDATECHECK_INTERVAL,
	OPTION_UPDATECHECK_LASTDATE,
	OPTION_UPDATECHECK_LASTVERSION,
	OPTION_UPDATECHECK_NEWVERSION,
	OPTION_UPDATECHECK_CHECKBETA,
	OPTION_DEBUG_MENU,
	OPTION_FILEEXISTS_DOWNLOAD,
	OPTION_FILEEXISTS_UPLOAD,
	OPTION_ASCIIRESUME,
	OPTION_GREETINGVERSION,
	OPTION_GREETINGRESOURCES,
	OPTION_ONETIME_DIALOGS,
	OPTION_SHOW_TREE_LOCAL,
	OPTION_SHOW_TREE_REMOTE,
	OPTION_FILEPANE_LAYOUT,
	OPTION_FILEPANE_SWAP,
	OPTION_FILELIST_DIRSORT,
	OPTION_FILELIST_NAMESORT,
	OPTION_QUEUE_SUCCESSFUL_AUTOCLEAR,
	OPTION_QUEUE_COLUMN_WIDTHS,
	OPTION_LOCALFILELIST_COLUMN_WIDTHS,
	OPTION_REMOTEFILELIST_COLUMN_WIDTHS,
	OPTION_MAINWINDOW_POSITION,
	OPTION_MAINWINDOW_SPLITTER_POSITION,
	OPTION_LOCALFILELIST_SORTORDER,
	OPTION_REMOTEFILELIST_SORTORDER,
	OPTION_TIME_FORMAT,
	OPTION_DATE_FORMAT,
	OPTION_SHOW_MESSAGELOG,
	OPTION_SHOW_QUEUE,
	OPTION_EDIT_DEFAULTEDITOR,
	OPTION_EDIT_ALWAYSDEFAULT,
	OPTION_EDIT_CUSTOMASSOCIATIONS,
	OPTION_COMPARISONMODE,
	OPTION_COMPARISON_THRESHOLD,
	OPTION_SITEMANAGER_POSITION,
	OPTION_ICONS_THEME,
	OPTION_ICONS_SCALE,
	OPTION_MESSAGELOG_TIMESTAMP,
	OPTION_SITEMANAGER_LASTSELECTED,
	OPTION_LOCALFILELIST_COLUMN_SHOWN,
	OPTION_REMOTEFILELIST_COLUMN_SHOWN,
	OPTION_LOCALFILELIST_COLUMN_ORDER,
	OPTION_REMOTEFILELIST_COLUMN_ORDER,
	OPTION_FILELIST_STATUSBAR,
	OPTION_FILTERTOGGLESTATE,
	OPTION_SHOW_QUICKCONNECT,
	OPTION_MESSAGELOG_POSITION,
	OPTION_DOUBLECLICK_ACTION_FILE,
	OPTION_DOUBLECLICK_ACTION_DIRECTORY,
	OPTION_MINIMIZE_TRAY,
	OPTION_SEARCH_COLUMN_WIDTHS,
	OPTION_SEARCH_COLUMN_SHOWN,
	OPTION_SEARCH_COLUMN_ORDER,
	OPTION_SEARCH_SIZE,
	OPTION_COMPARE_HIDEIDENTICAL,
	OPTION_SEARCH_SORTORDER,
	OPTION_EDIT_TRACK_LOCAL,
	OPTION_PREVENT_IDLESLEEP,
	OPTION_FILTEREDIT_SIZE,
	OPTION_INVALID_CHAR_REPLACE_ENABLE,
	OPTION_INVALID_CHAR_REPLACE,
	OPTION_ALREADYCONNECTED_CHOICE,
	OPTION_EDITSTATUSDIALOG_SIZE,
	OPTION_SPEED_DISPLAY,
	OPTION_TOOLBAR_HIDDEN,
	OPTION_STRIP_VMS_REVISION,
	OPTION_STARTUP_ACTION,
	OPTION_PROMPTPASSWORDSAVE,
	OPTION_PERSISTENT_CHOICES,
	OPTION_QUEUE_COMPLETION_ACTION,
	OPTION_QUEUE_COMPLETION_COMMAND,
	OPTION_DND_DISABLED,
	OPTION_DISABLE_UPDATE_FOOTER,
	OPTION_MASTERPASSWORDENCRYPTOR,
	OPTION_TAB_DATA,

	// Has to be last element
	OPTIONS_NUM
};

unsigned int register_interface_options();

optionsIndex mapOption(interfaceOptions opt);

std::wstring GetEnv(char const* name);

class CXmlFile;
class COptions final : public wxEvtHandler, public COptionsBase
{
public:
	static COptions* Get();
	static void Init();
	static void Destroy();

	void Import(pugi::xml_node & element);

	void Save(bool processChanged = true);

	static CLocalPath GetUnadjustedSettingsDir();
	CLocalPath GetCacheDirectory();

	bool Cleanup(); // Removes all unknown elements from the XML

protected:
	COptions();
	virtual ~COptions();

	void Load(pugi::xml_node & settings, bool predefined, bool importing);

	pugi::xml_node CreateSettingsXmlElement();

	void LoadGlobalDefaultOptions();
	CLocalPath InitSettingsDir();

	virtual void process_changed(watched_options const& changed) override;
	void set_xml_value(pugi::xml_node & settings, size_t opt, bool clean);

	virtual void notify_changed() override;

	bool dirty_{};
	std::unique_ptr<CXmlFile> xmlFile_;

	static COptions* m_theOptions;

	wxTimer m_save_timer;

	DECLARE_EVENT_TABLE()
	void OnTimer(wxTimerEvent& event);
};

#endif
