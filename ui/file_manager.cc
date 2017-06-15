//
// PROJECT:         Aspia Remote Desktop
// FILE:            ui/file_manager.cc
// LICENSE:         See top-level directory
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "ui/file_manager.h"
#include "ui/resource.h"
#include "ui/base/module.h"
#include "base/logging.h"

namespace aspia {

static const int kDefaultWindowWidth = 800;
static const int kDefaultWindowHeight = 600;

FileManager::FileManager(Delegate* delegate) :
    delegate_(delegate)
{
    ui_thread_.Start(MessageLoop::TYPE_UI, this);
}

FileManager::~FileManager()
{
    ui_thread_.Stop();
}

void FileManager::OnBeforeThreadRunning()
{
    runner_ = ui_thread_.message_loop_proxy();
    DCHECK(runner_);

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
        WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

    if (!Create(nullptr, style, L"File Transfer"))
    {
        LOG(ERROR) << "File manager window not created";
        runner_->PostQuit();
    }
    else
    {
        ScopedHICON icon(Module::Current().icon(IDI_MAIN,
                                                GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CYSMICON),
                                                LR_CREATEDIBSECTION));
        SetIcon(icon);
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
    }
}

void FileManager::OnAfterThreadRunning()
{
    DestroyWindow();
}

void FileManager::AddDriveItem(PanelType panel_type,
                               proto::DriveListItem::Type drive_type,
                               const std::wstring& drive_path,
                               const std::wstring& drive_name)
{
    if (panel_type == FileManager::PanelType::REMOTE)
    {
        remote_panel_.AddDriveItem(drive_type, drive_path, drive_name);
    }
    else
    {
        DCHECK(panel_type == FileManager::PanelType::LOCAL);
        local_panel_.AddDriveItem(drive_type, drive_path, drive_name);
    }
}

void FileManager::AddDirectoryItem(PanelType panel_type,
                                   proto::DirectoryListItem::Type item_type,
                                   const std::wstring& item_name,
                                   uint64_t item_size)
{
    if (panel_type == FileManager::PanelType::REMOTE)
    {

    }
    else
    {
        DCHECK(panel_type == FileManager::PanelType::LOCAL);
    }
}

void FileManager::OnDriveListRequest(FileManager::PanelType panel_type)
{
    delegate_->OnDriveListRequest(panel_type);
}

void FileManager::OnCreate()
{
    splitter_.CreateWithProportion(hwnd());

    local_panel_.CreatePanel(splitter_, FileManager::PanelType::LOCAL, this);
    remote_panel_.CreatePanel(splitter_, FileManager::PanelType::REMOTE, this);

    splitter_.SetPanels(local_panel_, remote_panel_);

    MoveWindow(hwnd(), 0, 0, kDefaultWindowWidth, kDefaultWindowHeight, TRUE);
    CenterWindow();

    runner_->PostTask(std::bind(&FileManager::OnDriveListRequest,
                                this,
                                FileManager::PanelType::LOCAL));

    runner_->PostTask(std::bind(&FileManager::OnDriveListRequest,
                                this,
                                FileManager::PanelType::REMOTE));
}

void FileManager::OnDestroy()
{
    local_panel_.DestroyWindow();
    remote_panel_.DestroyWindow();
    splitter_.DestroyWindow();
}

void FileManager::OnSize(int width, int height)
{
    HDWP dwp = BeginDeferWindowPos(1);

    if (dwp)
    {
        DeferWindowPos(dwp,
                       splitter_,
                       nullptr,
                       0,
                       0,
                       width,
                       height,
                       SWP_NOACTIVATE | SWP_NOZORDER);

        EndDeferWindowPos(dwp);
    }
}

void FileManager::OnGetMinMaxInfo(LPMINMAXINFO mmi)
{
    mmi->ptMinTrackSize.x = 500;
    mmi->ptMinTrackSize.y = 400;
}

void FileManager::OnClose()
{
    delegate_->OnWindowClose();
}

bool FileManager::OnMessage(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* result)
{
    switch (msg)
    {
        case WM_CREATE:
            OnCreate();
            break;

        case WM_SIZE:
            OnSize(LOWORD(lparam), HIWORD(lparam));
            break;

        case WM_GETMINMAXINFO:
            OnGetMinMaxInfo(reinterpret_cast<LPMINMAXINFO>(lparam));
            break;

        case WM_NOTIFY:
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wparam))
            {
                case 0:
                    break;
            }
        }
        break;

        case WM_CLOSE:
            OnClose();
            break;

        case WM_DESTROY:
            OnDestroy();
            break;

        default:
            return false;
    }

    *result = 0;
    return true;
}

} // namespace aspia