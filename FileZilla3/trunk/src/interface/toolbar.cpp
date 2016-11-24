#include <filezilla.h>
#include "filter.h"
#include "listingcomparison.h"
#include "Mainfrm.h"
#include "Options.h"
#include "QueueView.h"
#include "themeprovider.h"
#include "toolbar.h"
#include "xh_toolb_ex.h"

IMPLEMENT_DYNAMIC_CLASS(CToolBar, wxToolBar)

CToolBar::~CToolBar()
{
	for (auto iter = m_hidden_tools.begin(); iter != m_hidden_tools.end(); ++iter) {
		delete iter->second;
	}
}

CToolBar* CToolBar::Load(CMainFrame* pMainFrame)
{
	wxSize iconSize = CThemeProvider::GetIconSize(iconSizeSmall, true);
#ifdef __WXMAC__
	// OS X only knows two hardcoded toolbar sizes.
	if (iconSize.x >= 32) {
		iconSize = wxSize(32, 32);
	}
	else {
		iconSize = wxSize(24, 24);
	}
#endif
	wxToolBarXmlHandlerEx::SetIconSize(iconSize);

	CToolBar* toolbar = dynamic_cast<CToolBar*>(wxXmlResource::Get()->LoadToolBar(pMainFrame, _T("ID_TOOLBAR")));
	if (!toolbar) {
		return 0;
	}

	toolbar->m_pMainFrame = pMainFrame;

	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_REMOTE_IDLE, true);
	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_SERVER, true);
	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_SYNC_BROWSE, true);
	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_COMPARISON, true);
	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_APPLYFILTER, true);

	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_QUEUEPROCESSING, false);
	CContextManager::Get()->RegisterHandler(toolbar, STATECHANGE_CHANGEDCONTEXT, false);

	toolbar->RegisterOption(OPTION_SHOW_MESSAGELOG);
	toolbar->RegisterOption(OPTION_SHOW_QUEUE);
	toolbar->RegisterOption(OPTION_SHOW_TREE_LOCAL);
	toolbar->RegisterOption(OPTION_SHOW_TREE_REMOTE);
	toolbar->RegisterOption(OPTION_MESSAGELOG_POSITION);

	toolbar->ToggleTool(XRCID("ID_TOOLBAR_FILTER"), CFilterManager::HasActiveFilters());
	toolbar->ToggleTool(XRCID("ID_TOOLBAR_LOGVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG) != 0);
	toolbar->ToggleTool(XRCID("ID_TOOLBAR_QUEUEVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE) != 0);
	toolbar->ToggleTool(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_LOCAL) != 0);
	toolbar->ToggleTool(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_REMOTE) != 0);

	if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2) {
		toolbar->HideTool(XRCID("ID_TOOLBAR_LOGVIEW"));
	}

#ifdef __WXMAC__
	// Hide then re-show fixes some odd sizing
	toolbar->Hide();
	if (COptions::Get()->GetOptionVal(OPTION_TOOLBAR_HIDDEN) == 0)
		toolbar->Show();
#endif
	return toolbar;
}

#ifdef __WXMSW__
bool CToolBar::Realize()
{
	wxASSERT(HasFlag(wxTB_NOICONS));

	bool ret = wxToolBar::Realize();
	if (!ret) {
		return false;
	}

	wxSize const size = GetToolBitmapSize();
	wxASSERT(size.x > 0 && size.y > 0);
	auto toolImages = std::make_unique<wxImageList>(size.x, size.y, false, 0);
	auto disabledToolImages = std::make_unique<wxImageList>(size.x, size.y, false, 0);

	HWND hwnd = GetHandle();

	auto hImgList = reinterpret_cast<HIMAGELIST>(toolImages->GetHIMAGELIST());
	auto hDisabledImgList = reinterpret_cast<HIMAGELIST>(disabledToolImages->GetHIMAGELIST());
	::SendMessage(hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImgList));
	::SendMessage(hwnd, TB_SETDISABLEDIMAGELIST, 0, reinterpret_cast<LPARAM>(hDisabledImgList));

	toolImages_ = std::move(toolImages);
	disabledToolImages_ = std::move(disabledToolImages);

	int images{};
	for (size_t i = 0; i < GetToolsCount(); ++i) {
		auto tool = GetToolByPos(static_cast<int>(i));
		if (!tool || tool->GetStyle() != wxTOOL_STYLE_BUTTON) {
			continue;
		}

		auto bmp = tool->GetBitmap();
		if (!bmp.IsOk()) {
			continue;
		}
			
		int image = toolImages_->Add(bmp);
		auto disabled = tool->GetDisabledBitmap();
		if (!disabled.IsOk()) {
			disabled = wxBitmap(bmp.ConvertToImage().ConvertToGreyscale());
		}
		disabledToolImages_->Add(disabled);

		TBBUTTONINFO btn{};
		btn.cbSize = sizeof(TBBUTTONINFO);
		btn.dwMask = TBIF_BYINDEX;
		int index = ::SendMessage(hwnd, TB_GETBUTTONINFO, i, reinterpret_cast<LPARAM>(&btn));
		if (index != i) {
			return false;
		}

		btn.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
		btn.iImage = image;
		if (::SendMessage(hwnd, TB_SETBUTTONINFO, i, reinterpret_cast<LPARAM>(&btn)) == 0) {
			return false;
		}
	}

	return true;
}

#endif

void CToolBar::OnStateChange(CState* pState, t_statechange_notifications notification, const wxString&, const void*)
{
	switch (notification)
	{
	case STATECHANGE_CHANGEDCONTEXT:
	case STATECHANGE_SERVER:
	case STATECHANGE_REMOTE_IDLE:
		UpdateToolbarState();
		break;
	case STATECHANGE_QUEUEPROCESSING:
		{
			const bool check = m_pMainFrame->GetQueue() && m_pMainFrame->GetQueue()->IsActive() != 0;
			ToggleTool(XRCID("ID_TOOLBAR_PROCESSQUEUE"), check);
		}
		break;
	case STATECHANGE_SYNC_BROWSE:
		{
			bool is_sync_browse = pState && pState->GetSyncBrowse();
			ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), is_sync_browse);
		}
		break;
	case STATECHANGE_COMPARISON:
		{
			bool is_comparing = pState && pState->GetComparisonManager()->IsComparing();
			ToggleTool(XRCID("ID_TOOLBAR_COMPARISON"), is_comparing);
		}
		break;
	case STATECHANGE_APPLYFILTER:
		ToggleTool(XRCID("ID_TOOLBAR_FILTER"), CFilterManager::HasActiveFilters());
		break;
	default:
		break;
	}
}

void CToolBar::UpdateToolbarState()
{
	CState* pState = CContextManager::Get()->GetCurrentContext();
	if (!pState)
		return;

	const CServer* pServer = pState->GetServer();
	const bool idle = pState->IsRemoteIdle();

	EnableTool(XRCID("ID_TOOLBAR_DISCONNECT"), pServer && idle);
	EnableTool(XRCID("ID_TOOLBAR_CANCEL"), pServer && !idle);
	EnableTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pServer != 0);

	ToggleTool(XRCID("ID_TOOLBAR_COMPARISON"), pState->GetComparisonManager()->IsComparing());
	ToggleTool(XRCID("ID_TOOLBAR_SYNCHRONIZED_BROWSING"), pState->GetSyncBrowse());

	bool canReconnect;
	if (pServer || !idle)
		canReconnect = false;
	else {
		canReconnect = static_cast<bool>(pState->GetLastSite().m_server);
	}
	EnableTool(XRCID("ID_TOOLBAR_RECONNECT"), canReconnect);
}

void CToolBar::OnOptionsChanged(changed_options_t const& options)
{
	if (options.test(OPTION_SHOW_MESSAGELOG)) {
		ToggleTool(XRCID("ID_TOOLBAR_LOGVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG) != 0);
	}
	if (options.test(OPTION_SHOW_QUEUE)) {
		ToggleTool(XRCID("ID_TOOLBAR_QUEUEVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_QUEUE) != 0);
	}
	if (options.test(OPTION_SHOW_TREE_LOCAL)) {
		ToggleTool(XRCID("ID_TOOLBAR_LOCALTREEVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_LOCAL) != 0);
	}
	if (options.test(OPTION_SHOW_TREE_REMOTE)) {
		ToggleTool(XRCID("ID_TOOLBAR_REMOTETREEVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_TREE_REMOTE) != 0);
	}
	if (options.test(OPTION_MESSAGELOG_POSITION)) {
		if (COptions::Get()->GetOptionVal(OPTION_MESSAGELOG_POSITION) == 2)
			HideTool(XRCID("ID_TOOLBAR_LOGVIEW"));
		else {
			ShowTool(XRCID("ID_TOOLBAR_LOGVIEW"));
			ToggleTool(XRCID("ID_TOOLBAR_LOGVIEW"), COptions::Get()->GetOptionVal(OPTION_SHOW_MESSAGELOG) != 0);
		}
	}
}

bool CToolBar::ShowTool(int id)
{
	int offset = 0;

	for (auto iter = m_hidden_tools.begin(); iter != m_hidden_tools.end(); ++iter) {
		if (iter->second->GetId() != id) {
			offset++;
			continue;
		}

		InsertTool(iter->first - offset, iter->second);
		Realize();
		m_hidden_tools.erase(iter);

		return true;
	}

	return false;
}

bool CToolBar::HideTool(int id)
{
	int pos = GetToolPos(id);
	if (pos == -1)
		return false;

	wxToolBarToolBase* tool = RemoveTool(id);
	if (!tool)
		return false;

	for (auto const& iter : m_hidden_tools) {
		if (iter.first > pos)
			break;

		++pos;
	}

	m_hidden_tools[pos] = tool;

	return true;
}
