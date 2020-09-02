#include <windows.h>

#define TARGET_PLATFORM TEXT("Win32")
#define TARGET_CPU TEXT("IA-32(x86)")
#define WND_CLASS_NAME TEXT("My_Window")
#define APP_SETFOCUS WM_APP

HWND hwnd, hbtn_ok, hbtn_term, hbtn_clr, hedi0, hedi1, hedi2, hedi_out, hwnd_temp, hwnd_focused;
HINSTANCE hInst; // Instance Handle �̃o�b�N�A�b�v
HMENU hmenu;
OPENFILENAME ofn;
DWORD dThreadID;
HANDLE hThread, hFile;
HDC hdc, hMemDC;
HFONT hMesFont, hFbtn, hFedi, hFnote; // �쐬����t�H���g
LOGFONT rLogfont; // �쐬����t�H���g�̍\����
HBRUSH hBrush, hBshSys; // �擾����u���V
HPEN hPen, hPenSys; // �擾����y��
PAINTSTRUCT ps;
RECT rect;
HBITMAP hBitmap;
INT scrx=0, scry=0, editlen=0, btnsize[2]={0};
LONGLONG num[3]; // ���͒l��t�p�ϐ�
bool threadcancelled=false, working=false, onlycnt=false, usefile=false, mode=false, overwrite=false;
TCHAR wctemp[1024]/*�����񍇐��E�󂯕t���Ɏg�p���鉼�ϐ�*/, wcmes[4][4096]/*���͂�ۑ�����z��*/, wcFile[MAX_PATH+1], wcEdit[65540];
CHAR cEdit[65540];
WNDPROC wpedi0_old, wpedi1_old, wpedi2_old; // ���� Window Procedure �̃A�h���X�i�[�p�ϐ�

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Edit0WindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Edit1WindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Edit2WindowProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI PrimeFactorization(LPVOID);
DWORD WINAPI ListPrimeNumbers(LPVOID);
void Paint();
void ChangeBackgroundColor();
void OutputInEditbox(HWND hWnd, LPCTSTR arg){ // �G�f�B�b�g�{�b�N�X�̖����ɕ������ǉ�
    int EditLen = (INT)SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(hWnd, EM_SETSEL, EditLen, EditLen);
    SendMessage(hWnd, EM_REPLACESEL, 0, (WPARAM)arg);
}
void FinalizeErrorLPN(){
    working = false;
    EnableWindow(hbtn_ok, TRUE);
    EnableWindow(hedi0, TRUE);
    EnableWindow(hedi1, TRUE);
    if(!onlycnt) EnableWindow(hedi2, TRUE);
    EnableWindow(hbtn_term, FALSE);
    EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_ENABLED); // �u�I�v�V�����v���j���[���ēx�L����
    DrawMenuBar(hwnd); // ���j���[�ĕ`��
    SetWindowText(hwnd, wcmes[0]);
    SendMessage(hwnd, APP_SETFOCUS, 0, 0);
    threadcancelled = false;
    return;
}

extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    hInst = hInstance;

    WNDCLASSEX wcl;
    wcl.cbSize = sizeof(WNDCLASSEX);
    wcl.hInstance = hInstance;
    wcl.lpszClassName = WND_CLASS_NAME;
    wcl.lpfnWndProc = WindowProc;
    wcl.style = NULL;
    wcl.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100)); // load the icon
    wcl.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(100)); // load the small icon
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.lpszMenuName = MAKEINTRESOURCE(1001); // load the menu
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    if(!RegisterClassEx(&wcl)) return FALSE;

    // ��ʃT�C�Y���擾
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    if(rect.bottom*5/3 > rect.right){
        scrx = rect.right*3/4;
        scry = rect.right*9/20;
    } else{
        scrx = rect.bottom*10/9;
        scry = rect.bottom*2/3;
    }
    if(scrx < 800 || scry < 480) {scrx = 800; scry = 480;}

    // default title
    wcscpy(wcmes[0], TEXT("�f���������v���O���� - ") TARGET_PLATFORM TEXT(" API ") TARGET_CPU);

    // "about" statement
    wcscpy(wcmes[1], TEXT("�f�������� for Windows PC Ver. 2\n\n")
        TEXT("�f���֘A�̌v�Z���s���V���v���ȃt���[�\�t�g�E�F�A�ł��B\n")
        TEXT("1�`9223372036854775807�̎��R���ɑΉ����Ă��܂��B\n")
        TEXT("\nwatamario�́A���̃\�t�g�E�F�A�̎g�p�ɂ���Ĕ��������v�Z���ʂ̌��A�o�O���ɂ��듮����܂�")
        TEXT("�����Ȃ鑹�Q�ɑ΂��Ă��⏞�v���܂���B���ȐӔC�ł����p���������B\n")
        TEXT("\n�J����: Borland C++ 5.5.1 for Win32\n")
        TEXT("�v���O�������: ") TARGET_PLATFORM TEXT(" Application\n")
        TEXT("CPU�A�[�L�e�N�`��: ") TARGET_CPU TEXT("\n")
        TEXT("�r���h����: ") __DATE__ TEXT(" ") __TIME__
        TEXT("\n\nCopyright (C) 2018-2020 watamario All rights reserved.")
    );

    // "help" statement for Prime Factorization
    wcscpy(wcmes[2], TEXT("���͂��ꂽ���R����f�����������܂��B�f������ɂ����p�ł��܂��B\n")
        TEXT("��̓��̓{�b�N�X�Ɏ��R������͂��AOK�{�^���܂���Enter�ŏ������J�n���܂��B\n")
        TEXT("����ȑf�����������R������������̂ɂ͎��Ԃ�������܂��B���܂�ɒ����A�҂ĂȂ��ꍇ�͒��f�{�^���Œ��f�ł��܂��B\n")
        TEXT("���ʋy�уG���[���e�́A��ʒ����̃{�b�N�X�ɏo�͂���܂��B\n")
        TEXT("��̃��j���[�o�[�̃I�v�V��������A�f���񋓁E�����グ�̋@�\�ɐ؂�ւ����܂��B\n")
    );

    // "help" statement for List Prime Numbers and Counting
    wcscpy(wcmes[3], TEXT("���͂��ꂽ�͈͂őf���̗񋓁A�����グ���s���܂��B\n")
        TEXT("��̊e���̓{�b�N�X�Ɏ��R������͂��AOK�{�^���܂���Enter�ŏ������J�n���܂��B\n")
        TEXT("�󔒂܂���0����͂���ƁA�������Ƃ��Ĉ����܂��B\n")
        TEXT("���͓��e�ɂ���Ă͂��Ȃ�̎��Ԃ�������ꍇ������܂��B���̏ꍇ�͒��f�{�^���Œ��f�ł��܂��B\n")
        TEXT("���ʋy�уG���[���e�́A��ʒ����̃{�b�N�X�ɏo�͂���܂��B\n")
        TEXT("��̃��j���[�o�[�̃I�v�V��������A�o�͂𒲐��ł��܂��B�e�L�X�g�t�@�C���o�͂��w�肷��ƁA�w�肵���ꏊ��UTF-16 LE�ŏo�͂���܂��B�Ȃ��A1000�𒴂��鐔��������Ƃ��Ďw�肵���ꍇ�͕K�����̕��@���w�肳��܂��B\n")
        TEXT("���l�ɃI�v�V��������A�f���������̋@�\�ɐ؂�ւ����܂��B\n")
    );

    hwnd = CreateWindowEx( // create the main window
        NULL,
        WND_CLASS_NAME,
        wcmes[0],
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, // WS_CLIPCHILDREN �Ŏq�E�B���h�E�̈��`��Ώۂ���O��(������h�~)
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        scrx,
        scry,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    hbtn_ok = CreateWindowEx( // ok button
        NULL,
        TEXT("BUTTON"),
        TEXT("OK"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        320,
        0,
        64,
        32,
        hwnd,
        (HMENU)0,
        hInstance,
        NULL);

    hbtn_term = CreateWindowEx( // terminate button
        NULL,
        TEXT("BUTTON"),
        TEXT("���f"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        384,
        0,
        64,
        32,
        hwnd,
        (HMENU)1,
        hInstance,
        NULL);

    hbtn_clr = CreateWindowEx( // clear the history button
        NULL,
        TEXT("BUTTON"),
        TEXT("��������"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        448,
        0,
        128,
        32,
        hwnd,
        (HMENU)2,
        hInstance,
        NULL);

    if(!hwnd) return FALSE;

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    MSG msg;
    BOOL bRet;
    SetTimer(hwnd, 1, 16, NULL);

    while( (bRet=GetMessage(&msg, hwnd, 0, 0)) ) { // continue unless the message is WM_QUIT(=0)
        if(bRet==-1) break; // when error
        else{
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_CREATE: // called at first
            hmenu = GetMenu(hWnd);
            CheckMenuRadioItem(hmenu, 2051, 2052, 2051, MF_BYCOMMAND);
            EnableMenuItem(hmenu, 2060, MF_BYCOMMAND | MFS_GRAYED);
            EnableMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_GRAYED);
            hMemDC = CreateCompatibleDC(NULL);
            hBshSys = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            hPenSys = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));

            hedi0 = CreateWindowEx( // input box
                NULL,
                TEXT("EDIT"),
                NULL,
                WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL,
                64, // x
                0, // y
                256, // x����
                32, // y����
                hWnd,
                (HMENU)50,
                ((LPCREATESTRUCT)(lParam))->hInstance,
                NULL);
            SendMessage(hedi0, EM_SETLIMITTEXT, (WPARAM)19, 0); // �����t��64�r�b�g�̌��E�����ɐݒ�
            wpedi0_old = (WNDPROC)SetWindowLongPtr(hedi0, GWLP_WNDPROC, (LONG_PTR)Edit0WindowProc); // Window Procedure�̂���ւ�
            SetFocus(hedi0); hwnd_focused = hedi0;

            hedi_out = CreateWindowEx( // results box
                NULL,
                TEXT("EDIT"),
                NULL,
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_LEFT | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                scrx/20,
                scry*3/10,
                scrx*9/10,
                scry*6/10,
                hWnd,
                (HMENU)60,
                ((LPCREATESTRUCT)(lParam))->hInstance,
                NULL);
            SendMessage(hedi_out, EM_SETLIMITTEXT, (WPARAM)65536, 0); // �������������ő�l�ɕύX
            break;

        case WM_CLOSE: // called when closing
            KillTimer(hWnd, 1); // stop the timer
            if(IDYES == MessageBox(hWnd, TEXT("�{���ɏI�����܂����H"), TEXT("�v���O�����̏I��"), MB_YESNO | MB_ICONINFORMATION)) {
                DestroyWindow(hWnd);
            } else{
                SetTimer(hWnd, 1, 16, NULL); // restart the timer
                SetFocus(hwnd_focused); // set focus on a input box
            }
            break;

        case WM_TIMER:
            ChangeBackgroundColor();
            if(!(hwnd_temp=GetFocus()) || (hwnd_temp!=hedi0 && hwnd_temp!=hedi1 && hwnd_temp!=hedi2)) break;
            hwnd_focused = hwnd_temp;
            break;

        case WM_PAINT:
            Paint();
            break;

        case WM_SIZE: // when the window size is changed
            GetClientRect(hWnd, &rect);
            scrx = rect.right; scry = rect.bottom;

            // create a font for the main window
            if(scrx/24 < scry/12) rLogfont.lfHeight = scrx/24;
            else rLogfont.lfHeight = scry/12;
            rLogfont.lfWidth = 0;
            rLogfont.lfEscapement = 0;
            rLogfont.lfOrientation = 0;
            rLogfont.lfWeight = FW_EXTRABOLD;
            rLogfont.lfItalic = TRUE;
            rLogfont.lfUnderline = TRUE;
            rLogfont.lfStrikeOut = FALSE;
            rLogfont.lfCharSet = SHIFTJIS_CHARSET;
            rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
            rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            rLogfont.lfQuality = DEFAULT_QUALITY;
            rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
            wsprintf(rLogfont.lfFaceName, TEXT("MS PGothic"));
            DeleteObject(hMesFont);
            hMesFont = CreateFontIndirect(&rLogfont); // �t�H���g���쐬

            // create a font for buttons
            if(24*scrx/700 < 24*scry/400) rLogfont.lfHeight = 24*scrx/700;
            else rLogfont.lfHeight = 24*scry/400;
            rLogfont.lfWidth = 0;
            rLogfont.lfEscapement = 0;
            rLogfont.lfOrientation = 0;
            rLogfont.lfWeight = FW_NORMAL;
            rLogfont.lfItalic = FALSE;
            rLogfont.lfUnderline = FALSE;
            rLogfont.lfStrikeOut = FALSE;
            rLogfont.lfCharSet = SHIFTJIS_CHARSET;
            rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
            rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            rLogfont.lfQuality = DEFAULT_QUALITY;
            rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
            wsprintf(rLogfont.lfFaceName, TEXT("MS PGothic"));
            DeleteObject(hFbtn);
            hFbtn = CreateFontIndirect(&rLogfont);

            // create a font for notes
            if(15*scrx/700 < 15*scry/400) rLogfont.lfHeight = 15*scrx/700;
            else rLogfont.lfHeight = 15*scry/400;
            DeleteObject(hFnote);
            hFnote = CreateFontIndirect(&rLogfont);

            // create a font for edit boxes
            if(16*scrx/700 < 16*scry/400) rLogfont.lfHeight = 16*scrx/700;
            else rLogfont.lfHeight = 16*scry/400;
            if(rLogfont.lfHeight<12) rLogfont.lfHeight=12;
            rLogfont.lfWidth = 0;
            rLogfont.lfEscapement = 0;
            rLogfont.lfOrientation = 0;
            rLogfont.lfWeight = FW_NORMAL;
            rLogfont.lfItalic = FALSE;
            rLogfont.lfUnderline = FALSE;
            rLogfont.lfStrikeOut = FALSE;
            rLogfont.lfCharSet = SHIFTJIS_CHARSET;
            rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
            rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            rLogfont.lfQuality = DEFAULT_QUALITY;
            rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
            wsprintf(rLogfont.lfFaceName, TEXT("MS Gothic"));
            DeleteObject(hFedi);
            hFedi = CreateFontIndirect(&rLogfont);

            // apply newly created fonts to objects
            SendMessage(hbtn_ok, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
            SendMessage(hbtn_term, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
            SendMessage(hbtn_clr, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
            SendMessage(hedi0, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
            SendMessage(hedi_out, WM_SETFONT, (WPARAM)hFedi, MAKELPARAM(FALSE, 0));
            if(mode){
                SendMessage(hedi1, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
                SendMessage(hedi2, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
            }

            // move and resize objects
            btnsize[0] = 64*scrx/700; btnsize[1] = 32*scry/400;
            MoveWindow(hedi_out, scrx/20, scry*3/10, scrx*9/10, scry*6/10, TRUE);
            if(!mode){
                MoveWindow(hedi0, btnsize[0], 0, btnsize[0]*4, btnsize[1], TRUE);
                MoveWindow(hbtn_ok, btnsize[0]*5, 0, btnsize[0], btnsize[1], TRUE);
                MoveWindow(hbtn_term, btnsize[0]*6, 0, btnsize[0], btnsize[1], TRUE);
                MoveWindow(hbtn_clr, btnsize[0]*7, 0, btnsize[0]*2, btnsize[1], TRUE);
            } else{
                MoveWindow(hedi0, btnsize[0], 0, btnsize[0]*4, btnsize[1], TRUE);
                MoveWindow(hedi1, btnsize[0]*6, 0, btnsize[0]*4, btnsize[1], TRUE);
                MoveWindow(hedi2, btnsize[0], btnsize[1], btnsize[0]*2, btnsize[1], TRUE);
                MoveWindow(hbtn_ok, btnsize[0]*3, btnsize[1], btnsize[0], btnsize[1], TRUE);
                MoveWindow(hbtn_term, btnsize[0]*4, btnsize[1], btnsize[0], btnsize[1], TRUE);
                MoveWindow(hbtn_clr, btnsize[0]*5, btnsize[1], btnsize[0]*2, btnsize[1], TRUE);
            }
            hBitmap = CreateCompatibleBitmap(hdc=GetDC(hWnd), rect.right, rect.bottom);
            ReleaseDC(hWnd, hdc);
            SelectObject(hMemDC, hBitmap); // update the memory device context in new size
            DeleteObject(hBitmap);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case WM_ACTIVATE:
            // set focus on the input box when the main window is activated
            if(LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) SetFocus(hwnd_focused);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case 0: // OK button
                    if(working) break;
                    working = true;
                    EnableWindow(hbtn_ok, FALSE);
                    EnableWindow(hedi0, FALSE);
                    EnableWindow(hedi1, FALSE);
                    EnableWindow(hedi2, FALSE);
                    EnableWindow(hbtn_term, TRUE);
                    EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_GRAYED); // �u�I�v�V�����v���j���[���O���[�A�E�g
                    DrawMenuBar(hwnd); // ���j���[�ĕ`��

                    SendMessage(hedi0, WM_GETTEXT, 31, (LPARAM)wctemp);
                    num[0]=_wtoi64(wctemp);
                    if(mode){
                        SendMessage(hedi1, WM_GETTEXT, 31, (LPARAM)wctemp);
                        num[1]=_wtoi64(wctemp);
                        SendMessage(hedi2, WM_GETTEXT, 31, (LPARAM)wctemp);
                        num[2]=_wtoi64(wctemp);
                    }
                    SetWindowText(hwnd, TEXT("�v�Z��...���΂炭���҂�������..."));

                    if(!mode) hThread = CreateThread(NULL, 0, PrimeFactorization, NULL, 0, &dThreadID);
                    else hThread = CreateThread(NULL, 0, ListPrimeNumbers, NULL, 0, &dThreadID);
                    break;

                case 1: // terminate button
                    threadcancelled = true;
                    break;

                case 2: // clear the history 
                    editlen = (INT)SendMessage(hedi_out, WM_GETTEXTLENGTH, 0, 0);
                    SendMessage(hedi_out, EM_SETSEL, 0, editlen);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT(""));
                    break;

                case 2001: // save
                    wcFile[0]=L'\0'; wcEdit[0]=L'\0';
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = TEXT("UTF-16 LE TEXT (*.txt)\0*.txt\0")
                                      TEXT("All files (*.*)\0*.*\0");
                    ofn.lpstrFile = wcFile;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrDefExt = TEXT(".txt");
                    ofn.lpstrTitle = TEXT("�ۑ���̎w��");
                    ofn.Flags = OFN_OVERWRITEPROMPT;
                    if(!GetSaveFileName(&ofn)) break;

                    hFile = CreateFile(
                        wcFile,
                        GENERIC_WRITE,
                        FILE_SHARE_READ, // ���v���O�����ɂ�Read�͋���
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
                    if(hFile == INVALID_HANDLE_VALUE){
                        MessageBox(hWnd, TEXT("�t�@�C���̍쐬�܂��̓I�[�v���Ɏ��s���܂����B\n�������݋֎~�ɂȂ��Ă��Ȃ������m�F���Ă��������B"), TEXT("�G���["), MB_OK | MB_ICONWARNING);
                        break;
                    }

                    GetWindowText(hedi_out, wcEdit, GetWindowTextLength(hedi_out)+1);
                    if(WriteFile(hFile, "\xFF\xFE", 2, NULL, NULL)){
                        MessageBox(hWnd, TEXT("�o�̓{�b�N�X�̓��e���e�L�X�g�t�@�C���ɏ����o���܂����B"), TEXT("���"), MB_OK | MB_ICONINFORMATION);
                    }else{
                        MessageBox(hWnd, TEXT("�o�̓t�@�C���ւ̏������݂Ɏ��s���܂����B\n�ʂ̃p�X�ōĎ��s���Ă��������B"), TEXT("�G���["), MB_OK | MB_ICONWARNING);}
                    WriteFile(hFile, wcEdit, lstrlen(wcEdit)*sizeof(WCHAR), NULL, NULL);
                    CloseHandle(hFile);
                    break;

                case 2002: // copy
                    editlen = (INT)SendMessage(hedi_out, WM_GETTEXTLENGTH, 0, 0);
                    SendMessage(hedi_out, EM_SETSEL, 0, editlen);
                    SendMessage(hedi_out, WM_COPY, 0, 0);
                    MessageBox(hWnd, TEXT("�o�̓{�b�N�X�̓��e���N���b�v�{�[�h�ɃR�s�[���܂����B"), TEXT("���"), MB_OK | MB_ICONINFORMATION);
                    break;

                case 2009: // close menu in the menubar
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case 2101: // "help" menu in the menubar
                    MessageBox(hWnd, wcmes[(mode ? 3 : 2)], TEXT("�g����"), MB_OK | MB_ICONINFORMATION);
                    break;

                case 2109: // "about" menu in the menubar
                    MessageBox(hWnd, wcmes[1], TEXT("���̃v���O�����ɂ���"), MB_OK | MB_ICONINFORMATION);
                    break;

                case 2051: // "Prime Factorization" menu in the menubar
                    CheckMenuRadioItem(hmenu, 2051, 2052, 2051, MF_BYCOMMAND);
                    EnableMenuItem(hmenu, 2060, MF_BYCOMMAND | MFS_GRAYED);
                    EnableMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_GRAYED);
                    EnableMenuItem(hmenu, 2062, MF_BYCOMMAND | MFS_GRAYED);
                    DestroyWindow(hedi1); // mode1�Œǉ����ꂽ�G�f�B�b�g�{�b�N�X���폜
                    DestroyWindow(hedi2); // mode1�Œǉ����ꂽ�G�f�B�b�g�{�b�N�X���폜
                    mode = 0;
                    SendMessage(hwnd, WM_SIZE, 0, 0);
                    break;

                case 2052: // "List/Count Prime Numbers" menu in the menubar
                    CheckMenuRadioItem(hmenu, 2051, 2052, 2052, MF_BYCOMMAND);
                    EnableMenuItem(hmenu, 2060, MF_BYCOMMAND | MFS_ENABLED);
                    EnableMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_ENABLED);
                    EnableMenuItem(hmenu, 2062, MF_BYCOMMAND | MFS_ENABLED);
                    mode = 1;
                    hedi1 = CreateWindowEx( // input box
                        NULL,
                        TEXT("EDIT"),
                        NULL,
                        WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL,
                        0,
                        0,
                        0,
                        0,
                        hWnd,
                        (HMENU)51,
                        hInst,
                        NULL);
                    SendMessage(hedi1, EM_SETLIMITTEXT, (WPARAM)19, 0);
                    wpedi1_old = (WNDPROC)SetWindowLongPtr(hedi1, GWLP_WNDPROC, (LONG_PTR)Edit1WindowProc); // Window Procedure �̂���ւ�

                    hedi2 = CreateWindowEx( // input box
                        NULL,
                        TEXT("EDIT"),
                        NULL,
                        WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL,
                        0,
                        0,
                        0,
                        0,
                        hWnd,
                        (HMENU)52,
                        hInst,
                        NULL);
                    SendMessage(hedi2, EM_SETLIMITTEXT, (WPARAM)19, 0);
                    SendMessage(hwnd, WM_SIZE, 0, 0);
                    wpedi2_old = (WNDPROC)SetWindowLongPtr(hedi2, GWLP_WNDPROC, (LONG_PTR)Edit2WindowProc); // Window Procedure �̂���ւ�
                    break;

                case 2060: // "Show only number of them" menu in the menubar
                    if(GetMenuState(hmenu, 2060, MF_BYCOMMAND) & MFS_CHECKED) { // ���Ƀ`�F�b�N�����Ă����ꍇ
                        CheckMenuItem(hmenu, 2060, MF_BYCOMMAND | MFS_UNCHECKED);
                        EnableMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_ENABLED);
                        EnableWindow(hedi2, TRUE);
                        onlycnt = false;
                    } else{
                        CheckMenuItem(hmenu, 2060, MF_BYCOMMAND | MFS_CHECKED);
                        EnableMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_GRAYED);
                        EnableWindow(hedi2, FALSE);
                        onlycnt = true;
                    }
                    break;

                case 2061: // "Output in a text file" menu in the menubar
                    if(GetMenuState(hmenu, 2061, MF_BYCOMMAND) & MFS_CHECKED) {
                        CheckMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_UNCHECKED);
                        usefile = false;
                    } else{
                        CheckMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_CHECKED);
                        usefile = true;
                    }
                    break;

                case 2062: // "Overwrite" menu in the menubar
                    if(GetMenuState(hmenu, 2062, MF_BYCOMMAND) & MFS_CHECKED) {
                        CheckMenuItem(hmenu, 2062, MF_BYCOMMAND | MFS_UNCHECKED);
                        overwrite = false;
                    } else{
                        CheckMenuItem(hmenu, 2062, MF_BYCOMMAND | MFS_CHECKED);
                        overwrite = true;
                    }
                    break;
            }
            if(LOWORD(wParam)<50) SetFocus(hwnd_focused); // set focus on input box if the clicked control isn't a edit box 
            break;

        case APP_SETFOCUS:
            SetFocus(hwnd_focused);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// custom window procedure for the input box
LRESULT CALLBACK Edit0WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wpedi0_old); // ���� Window Procedure �ɖ߂�
            break;

        // Enter�Ŏ��sor���̃G�f�B�b�g�Ɉړ�, ���s�ΏۂłȂ����default�ɗ���(�Ԃɕʂ̃��b�Z�[�W���ꂿ�Ⴞ��!!)
        case WM_CHAR:
            switch((CHAR)wParam) {
                case VK_RETURN:
                    if(!working && !mode) {
                        working = true;
                        KillTimer(hWnd, 1);
                        EnableWindow(hbtn_ok, FALSE);
                        EnableWindow(hedi0, FALSE);
                        EnableWindow(hedi1, FALSE);
                        EnableWindow(hedi2, FALSE);
                        EnableWindow(hbtn_term, TRUE);
                        EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_GRAYED); // �u�I�v�V�����v���j���[���O���[�A�E�g
                        DrawMenuBar(hwnd); // ���j���[�ĕ`��

                        SendMessage(hedi0, WM_GETTEXT, 31, (LPARAM)wctemp);
                        num[0]=_wtoi64(wctemp);
                        if(mode){
                            SendMessage(hedi1, WM_GETTEXT, 31, (LPARAM)wctemp);
                            num[1]=_wtoi64(wctemp);
                            SendMessage(hedi2, WM_GETTEXT, 31, (LPARAM)wctemp);
                            num[2]=_wtoi64(wctemp);
                        }
                        SetWindowText(hwnd, TEXT("�v�Z��...���΂炭���҂�������..."));

                        if(!mode) hThread = CreateThread(NULL, 0, PrimeFactorization, NULL, 0, &dThreadID);
                        else hThread = CreateThread(NULL, 0, ListPrimeNumbers, NULL, 0, &dThreadID);
                    } else if(mode) SetFocus(hedi1);
                    return 0;
            }
        default:
            return CallWindowProc(wpedi0_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK Edit1WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wpedi1_old);
            break;

        case WM_CHAR:
            // Enter�Ŏ��̃G�f�B�b�g�Ɉړ�, ���s�ΏۂłȂ����default�ɗ���(��ɕʂ̃��b�Z�[�W���ꂿ�Ⴞ��!!)
            switch((CHAR)wParam) {
                case VK_RETURN:
                    SetFocus(hedi2);
                    return 0;
            }
        default:
            return CallWindowProc(wpedi1_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK Edit2WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wpedi2_old);
            break;

        case WM_CHAR:
            // Enter�Ōv�Z�J�n, ���s�ΏۂłȂ����default�ɗ���(�Ԃɕʂ̃��b�Z�[�W���ꂿ�Ⴞ��!!)
            switch((CHAR)wParam) {
                case VK_RETURN:
                    if(!working) {
                        working = true;
                        KillTimer(hWnd, 1);
                        EnableWindow(hbtn_ok, FALSE);
                        EnableWindow(hedi0, FALSE);
                        EnableWindow(hedi1, FALSE);
                        EnableWindow(hedi2, FALSE);
                        EnableWindow(hbtn_term, TRUE);
                        EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_GRAYED); // �u�I�v�V�����v���j���[���O���[�A�E�g
                        DrawMenuBar(hwnd); // ���j���[�ĕ`��

                        SendMessage(hedi0, WM_GETTEXT, 31, (LPARAM)wctemp);
                        num[0]=_wtoi64(wctemp);
                        if(mode){
                            SendMessage(hedi1, WM_GETTEXT, 31, (LPARAM)wctemp);
                            num[1]=_wtoi64(wctemp);
                            SendMessage(hedi2, WM_GETTEXT, 31, (LPARAM)wctemp);
                            num[2]=_wtoi64(wctemp);
                        }
                        SetWindowText(hwnd, TEXT("�v�Z��...���΂炭���҂�������..."));

                        if(!mode) hThread = CreateThread(NULL, 0, PrimeFactorization, NULL, 0, &dThreadID);
                        else hThread = CreateThread(NULL, 0, ListPrimeNumbers, NULL, 0, &dThreadID);
                    }
                    return 0;
            }
        default:
            return CallWindowProc(wpedi2_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

void Paint() {
    GetClientRect(hwnd, &rect);
    SelectObject(hMemDC, hPen); // �f�o�C�X�R���e�L�X�g�ƃy�����Ȃ�
    SelectObject(hMemDC, hBrush); // �f�o�C�X�R���e�L�X�g�ƃu���V���Ȃ�
    Rectangle(hMemDC, rect.left, rect.top, rect.right, rect.bottom); // �̈悢���ς��Ɏl�p�`��`��

    if(!mode){
        SelectObject(hMemDC, hPenSys);
        SelectObject(hMemDC, hBshSys);
        Rectangle(hMemDC, 0, 0, btnsize[0], btnsize[1]);

        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(0, 0, 0));
        SelectObject(hMemDC, hFnote); // �f�o�C�X�R���e�L�X�g�Ƀt�H���g��ݒ�
        rect.right=btnsize[0]; rect.bottom=btnsize[1];
        DrawText(hMemDC, TEXT("���R��:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else{
        SelectObject(hMemDC, hPenSys);
        SelectObject(hMemDC, hBshSys);
        Rectangle(hMemDC, 0, 0, btnsize[0], btnsize[1]);
        Rectangle(hMemDC, btnsize[0]*5, 0, btnsize[0]*6, btnsize[1]);
        Rectangle(hMemDC, 0, btnsize[1], btnsize[0], btnsize[1]*2);

        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(0, 0, 0));
        SelectObject(hMemDC, hFnote);
        rect.right=btnsize[0]; rect.bottom=btnsize[1];
        DrawText(hMemDC, TEXT("�ŏ��l:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        rect.left=btnsize[0]*5; rect.right=btnsize[0]*6;
        DrawText(hMemDC, TEXT("�ő�l:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        rect.left=0; rect.top=btnsize[1]; rect.bottom=btnsize[1]*2; rect.right=btnsize[0];
        DrawText(hMemDC, TEXT("�����:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SetBkMode(hMemDC, OPAQUE);
    SetBkColor(hMemDC, RGB(255, 255, 0));
    SetTextColor(hMemDC, RGB(0, 0, 255));
    SelectObject(hMemDC, hMesFont);
    rect.left=0; rect.right=scrx; rect.top=btnsize[1]*(mode ? 2 : 1); rect.bottom=scry*3/10;
    if(working) {
        DrawText(
            hMemDC,
            TEXT("�v�Z���ł�..."),
            -1,
            &rect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    } else if(!mode){
        DrawText(
            hMemDC,
            TEXT("�f�����������������R������͂��ĉ�����"),
            -1,
            &rect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    } else if(mode){
        DrawText(
            hMemDC,
            TEXT("�f����T���͈͂ƍő������͂��ĉ�����"),
            -1,
            &rect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }
    rect.left=0; rect.right=scrx; rect.top=0; rect.bottom=scry;

    hdc = BeginPaint(hwnd, &ps);
    BitBlt(hdc, 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, SRCCOPY);
    EndPaint(hwnd, &ps);
    return;
}

void ChangeBackgroundColor() {
    static short r=0, g=255, b=255;
    if(b <= 0 && g < 255) g++; //
    if(g >= 255 && r > 0) r--; //
    if(r <= 0 && b < 255) b++; //
    if(b >= 255 && g > 0) g--; //
    if(g <= 0 && r < 255) r++; //
    if(r >= 255 && b > 0) b--; //
                                //
    if(r > 255) r = 255;		//
    if(r < 0) r = 0;			//
    if(g > 255) g = 255;		//
    if(g < 0) g = 0;			//
    if(b > 255) b = 255;	 	//
    if(b < 0) b = 0;			//
    DeleteObject(hBrush);
    DeleteObject(hPen);
    hBrush = CreateSolidBrush(RGB(r, g, b)); // rgb�u���V���쐬(�h��Ԃ��p)
    hPen = CreatePen(PS_SOLID, 1, RGB(r, g, b)); // rgb�y�����쐬(�֊s�p)
    InvalidateRect(hwnd, NULL, FALSE);
    return;
}

DWORD WINAPI PrimeFactorization(LPVOID arg) {
    LONGLONG N=num[0], cnt=0, i=2;
    bool chk=0;
    TCHAR wcresult[1024] = TEXT(""), wcstr[1024] = TEXT("");

    if(N<=0) { // �����Ȓl�������ꍇ
        KillTimer(hwnd, 1);
        SetWindowText(hwnd, wcmes[0]);
        MessageBox(hwnd, TEXT("�����Ȓl�����͂���܂����\n���R��(������)����͂��Ă��邩�m�F���Ă��������\n�傫�����鐔����͂����ꍇ�ं��̃G���[����������ꍇ������܂��"), TEXT("�G���["), MB_OK | MB_ICONWARNING);
        OutputInEditbox(hedi_out, TEXT("�G���[: �����Ȓl(���R���̂ݑΉ�)\r\n"));
        working = false;
        EnableWindow(hbtn_ok, TRUE);
        EnableWindow(hedi0, TRUE);
        EnableWindow(hbtn_term, FALSE);
        EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_ENABLED); // �u�I�v�V�����v���j���[���ēx�L����
        DrawMenuBar(hwnd); // ���j���[�ĕ`��
        InvalidateRect(hwnd, NULL, FALSE);
        SendMessage(hwnd, APP_SETFOCUS, 0, 0);
        SetTimer(hwnd, 1, 16, NULL);
        return 0;
    }

    // �v�Z
    while(1){
        while(i<=N) {
            if(threadcancelled) break;
            if(N%i==0 && N!=i) {
                chk = true;
                break;
            }
            if(N/i<i || N==i) {
                chk = false;
                break;
            }
            if(i==2) i++;
            else i+=2;
        }
        if(threadcancelled) break;
        if(chk) {
            wsprintf(wcstr, TEXT("%I64dx"), i); // �������f�����𕶎���ɕϊ��E�ux�v�𑫂�
            wcscat(wcresult, wcstr); // ���ʕ�����ɒǉ�
        } else {
            wsprintf(wcstr, TEXT("%I64d"), N); // ���̐����g�𕶎���ɕϊ�(�f���������ꍇ)
            wcscat(wcresult, wcstr); // ���ʕ�����ɒǉ�
            break;
        }
        N/=i;
        cnt++;
    }
    if(cnt==0 && N>1) wcscat(wcresult, TEXT(" (�f��)")); // �f���������ꍇ�A���̎|��ǉ�

    if(threadcancelled) { // �v�Z���f�������ꂽ�ꍇ�̏���
        working = false;
        EnableWindow(hbtn_ok, TRUE);
        EnableWindow(hedi0, TRUE);
        EnableWindow(hbtn_term, FALSE);
        EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_ENABLED); // �u�I�v�V�����v���j���[���ēx�L����
        DrawMenuBar(hwnd); // ���j���[�ĕ`��
        SetWindowText(hwnd, wcmes[0]);
        SendMessage(hwnd, APP_SETFOCUS, 0, 0);
        threadcancelled = false;
        return 0;
    }

    // ���ʕ�����̐���
    wsprintf(wcstr, TEXT("����: %I64d = %s"), num[0], wcresult);

    // update the Window's title and the result box
    OutputInEditbox(hedi_out, wcstr);
    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n")); // �����ď������ނȂ炱��1�s�����ŗǂ�
    wcscat(wcstr, TEXT(" - �f���������v���O����"));
    SetWindowText(hwnd, wcstr);

    working = false;
    EnableWindow(hbtn_ok, TRUE);
    EnableWindow(hedi0, TRUE);
    EnableWindow(hbtn_term, FALSE);
    EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_ENABLED); // �u�I�v�V�����v���j���[���ēx�L����
    DrawMenuBar(hwnd); // ���j���[�ĕ`��
    InvalidateRect(hwnd, NULL, FALSE);
    SendMessage(hwnd, APP_SETFOCUS, 0, 0);
    return 0;
}

DWORD WINAPI ListPrimeNumbers(LPVOID arg) {
    LONGLONG cnt=0, i, j;
    DWORD ret;
    TCHAR wcresult[1024] = TEXT(""), wcstr[1024] = TEXT(""), strFile[MAX_PATH+1] = TEXT("");
    HANDLE hfile = NULL;
    OPENFILENAME ofn = {0};

    // ���͒l�𒲐�(��ɋ�or"0"�̎��̑Ή�)
    if(num[0]<2) num[0]=2;
    if(num[0]!=2 && !(num[0]%2)) num[0]++;
    if(num[1]<1) num[1]=MAXLONGLONG;
    if(num[2]<1) num[2]=MAXLONGLONG;
    if(num[1]-num[0]<1) { // �����Ȓl�������ꍇ
        KillTimer(hwnd, 1);
        SetWindowText(hwnd, wcmes[0]);
        MessageBox(hwnd, TEXT("�����Ȓl�����͂���܂����\n�傫�����鐔����͂����ꍇ�ं��̃G���[����������ꍇ������܂��"), TEXT("�G���["), MB_OK | MB_ICONWARNING);
        OutputInEditbox(hedi_out, TEXT("�G���[: �����Ȓl\r\n"));
        FinalizeErrorLPN();
        SetTimer(hwnd, 1, 16, NULL);
        return 0;
    }

    // ��������傫���Ƃ��̑Ή�
    if(num[2]>1000 && !usefile){
        if(IDYES == MessageBox(hwnd,
            TEXT("��������傫���A�G�f�B�b�g�{�b�N�X�Ɍ��ʂ����܂�Ȃ��\��������܂��B\n����Ƀe�L�X�g�t�@�C���o�͂��I������܂����A���s���܂����H"),
            TEXT("�m�F"),
            MB_YESNO | MB_ICONINFORMATION)
            ) {
            usefile=1;
            CheckMenuItem(hmenu, 2061, MF_BYCOMMAND | MFS_CHECKED);
        } else{
            FinalizeErrorLPN();
            return 0;
        }
    }

    // �t�@�C���o�͂̂Ƃ��̉�����(�u���̂݁v�̂Ƃ��̓e�L�X�g�o�͂��Ȃ��̂ŊO���B�ȉ����l�B)
    if(!onlycnt && usefile){
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = TEXT("UTF-16 LE TEXT (*.txt)\0*.txt\0")
                          TEXT("All files (*.*)\0*.*\0");
        ofn.lpstrFile = strFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = TEXT(".txt");
        ofn.lpstrTitle = TEXT("���ʂ̏o�͐�w��");
        ofn.Flags = ( overwrite ? OFN_OVERWRITEPROMPT : NULL); // �����t�@�C���̏ꍇ�A�㏑�����[�h�̂Ƃ��̂݌x��
        if(!GetSaveFileName(&ofn)){
            SetWindowText(hwnd, wcmes[0]);
            FinalizeErrorLPN();
            return 0;
        }

        hfile = CreateFile(
            strFile,
            GENERIC_WRITE,
            FILE_SHARE_READ, // ���v���O�����ɂ�Read�͋���
            NULL,
            (overwrite ? CREATE_ALWAYS : OPEN_ALWAYS), // overwrite�L���Ȃ�u�����蒼���v�A�����Ȃ�u�J����������΍��v
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if(hfile == INVALID_HANDLE_VALUE){
            KillTimer(hwnd, 1);
            SetWindowText(hwnd, wcmes[0]);
            MessageBox(hwnd, TEXT("�t�@�C���̍쐬�܂��̓I�[�v���Ɏ��s���܂����B\n�������݋֎~�ɂȂ��Ă��Ȃ������m�F���Ă��������B"), TEXT("�G���["), MB_OK | MB_ICONWARNING);
            OutputInEditbox(hedi_out, TEXT("�G���[: �t�@�C���̍쐬�܂��̓I�[�v���Ɏ��s\r\n"));
            FinalizeErrorLPN();
            SetTimer(hwnd, 1, 16, NULL);
            return 0;
        }
        if( (ret=SetFilePointer(hfile, 0, NULL, FILE_END)) ){ // �t�@�C��������File Pointer���ړ� & �r�������߂��炩����
            if(ret==0xFFFFFFFF){
                KillTimer(hwnd, 1);
                SetWindowText(hwnd, wcmes[0]);
                MessageBox(hwnd, TEXT("�t�@�C���|�C���^�̐ݒ�Ɏ��s���܂����B"), TEXT("�G���["), MB_OK | MB_ICONWARNING);
                OutputInEditbox(hedi_out, TEXT("�G���[: �e�L�X�g�t�@�C���o�͎��s\r\n"));
                CloseHandle(hfile);
                FinalizeErrorLPN();
                SetTimer(hwnd, 1, 16, NULL);
                return 0;
            }
            ret=WriteFile(hfile, TEXT("\r\n\r\n"), lstrlen(TEXT("\r\n\r\n"))*sizeof(TCHAR), NULL, NULL); // �ǋL�Ȃ�2����s
        }else ret=WriteFile(hfile, "\xFF\xFE", 2, NULL, NULL); // ���߂Ȃ�Byte Order Mark
        if(!ret){
            KillTimer(hwnd, 1);
            SetWindowText(hwnd, wcmes[0]);
            MessageBox(hwnd, TEXT("�o�̓t�@�C���ւ̏������݂Ɏ��s���܂����B\n�ʂ̃p�X�ōĎ��s���Ă��������B"), TEXT("�G���["), MB_OK | MB_ICONWARNING);
            OutputInEditbox(hedi_out, TEXT("�G���[: �e�L�X�g�t�@�C���������ݎ��s\r\n"));
            CloseHandle(hfile);
            FinalizeErrorLPN();
            SetTimer(hwnd, 1, 16, NULL);
            return 0;
        }
    }

    if(!onlycnt && !usefile) OutputInEditbox(hedi_out, TEXT("����: "));
    else if(!onlycnt && usefile){
        WriteFile(hfile, TEXT("����: "), lstrlen(TEXT("����: "))*sizeof(WCHAR), NULL, NULL);
        OutputInEditbox(hedi_out, TEXT("�v�Z���ʂ��e�L�X�g�t�@�C���ɏo�͂��Ă��܂�...\r\n"));
    }

    // �v�Z
    for(i=num[0]; i<=num[1] && cnt<num[2]; i++){
        for(j=2; j<=i; j++) {
            if(threadcancelled) break;
            if(i%j==0 && i!=j) break;
            if(i/j<j || i==j) {
                if(!onlycnt){
                    wcresult[0] = L'\0';
                    if(cnt) wsprintf(wcresult, TEXT(", %I64d"), i);
                    else wsprintf(wcresult, TEXT("%I64d"), i);
                    if(usefile) WriteFile(hfile, wcresult, lstrlen(wcresult)*sizeof(WCHAR), NULL, NULL);
                    else OutputInEditbox(hedi_out, wcresult);
                }
                cnt++;
                break;
            }
            if(j!=2) j++; // 2�ȊO�Ȃ����1�X�ɑ��₷
        }
        if(threadcancelled) break;
        if(i!=2) i++; // 2�ȊO�Ȃ����1�X�ɑ��₷
    }

    // �v�Z���f�������ꂽ�ꍇ�̏���
    if(threadcancelled) {
        if(!onlycnt && !usefile) OutputInEditbox(hedi_out, TEXT("\r\n"));
        else if(!onlycnt && usefile){
            WriteFile(hfile, TEXT("\r\n"), lstrlen(TEXT("\r\n"))*sizeof(WCHAR), NULL, NULL);
            CloseHandle(hfile);
        }
        FinalizeErrorLPN();
        return 0;
    }

    // �Ō�̏o�͂Ȃ�
    if(!onlycnt && !usefile) OutputInEditbox(hedi_out, TEXT("\r\n"));
    else if(!onlycnt && usefile) WriteFile(hfile, TEXT("\r\n"), lstrlen(TEXT("\r\n"))*sizeof(WCHAR), NULL, NULL);
    wsprintf(wcstr, TEXT("���v��: %I64d (�͈�: %I64d�ȏ�%I64d�ȉ�, �����: %I64d)"), cnt, num[0], num[1], num[2]);
    if(!onlycnt && usefile){
        WriteFile(hfile, wcstr, lstrlen(wcstr)*sizeof(WCHAR), NULL, NULL);
        WriteFile(hfile, TEXT("\r\n"), lstrlen(TEXT("\r\n"))*sizeof(WCHAR), NULL, NULL);
        CloseHandle(hfile);
    }
    OutputInEditbox(hedi_out, wcstr);
    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n")); // '\r': CR, '\n': LF
    wcscat(wcstr, TEXT(" - �f���������v���O����"));
    SetWindowText(hwnd, wcstr);

    working = false;
    EnableWindow(hbtn_ok, TRUE);
    EnableWindow(hedi0, TRUE);
    EnableWindow(hedi1, TRUE);
    if(!onlycnt) EnableWindow(hedi2, TRUE);
    EnableWindow(hbtn_term, FALSE);
    EnableMenuItem(hmenu, 1, MF_BYPOSITION | MFS_ENABLED); // �u�I�v�V�����v���j���[���ēx�L����
    DrawMenuBar(hwnd); // ���j���[�ĕ`��
    InvalidateRect(hwnd, NULL, FALSE);
    SendMessage(hwnd, APP_SETFOCUS, 0, 0);
    return 0;
}