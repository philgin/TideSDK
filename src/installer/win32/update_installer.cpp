/**
 * Copyright (c) 2012 - 2014 TideSDK contributors 
 * http://www.tidesdk.org
 * Includes modified sources under the Apache 2 License
 * Copyright (c) 2008 - 2012 Appcelerator Inc
 * Refer to LICENSE for details of distribution and use.
 **/

#include <tideutils/application.h>
#include <tideutils/boot_utils.h>
#include <tideutils/file_utils.h>
#include <tideutils/win/win32_utils.h>
using namespace TideUtils;
using std::wstring;
using std::string;

#include <windows.h>
#include <cmath>
#include <sstream>
#include <string>
#include "progress_dialog.h"
#include "common.h"

ProgressDialog* dialog = 0;
HWND GetInstallerHWND()
{
    if (!dialog)
        return 0;

    return dialog->GetWindowHandle();
}

bool Progress(SharedDependency dependency, int percent)
{
    if (dialog->IsCancelled())
        return false;

    std::wstringstream str;
    str << L"About " << percent << L"%" << " done";
    dialog->SetLineText(3, str.str(), false);
    dialog->Update(percent, 100);
    return true;
}

static void Cleanup()
{
    if (dialog)
    {
        dialog->Hide();
        delete dialog;
    }
        
    CoUninitialize();
}

static void ExitWithError(const std::string& message, int exitCode)
{
    ShowError(message);
    Cleanup();
    exit(exitCode);
}

static wstring GetDependencyDescription(SharedDependency d)
{
    string result;

    if (d->type == MODULE)
        result = d->name + " module";
    else if (d->type == SDK)
        result = " SDK";
    else if (d->type == MOBILESDK)
        result = " Mobile SDK";
    else if (d->type == APP_UPDATE)
        result = " application update";
    else
        result = " runtime";

    result.append(" (");
    result.append(d->version);
    result.append(")");

    return UTF8ToWide(result);
}

static bool HandleAllJobs(SharedApplication app, vector<SharedDependency>& jobs)
{
    for (size_t i = 0; i < jobs.size(); i++)
    {
        SharedDependency dep(jobs.at(i));
        wstring description(GetDependencyDescription(dep));

        wstring text(L"Downloading the ");
        text.append(description);
        dialog->SetLineText(2, text, false);
        if (!DownloadDependency(app, dep))
            return false;

        text.assign(L"Extracting the ");
        text.append(description);
        dialog->SetLineText(2, text, false);
        if (!InstallDependency(app, dep))
            return false;
    }

    return true;
}

void ParseCommandLineDependencies(wstring toSearch, vector<SharedDependency>& jobs)
{
    string utf8ToSearch(WideToUTF8(toSearch));

    vector<string> tokens;
    FileUtils::Tokenize(utf8ToSearch, tokens, ",");
    for (size_t i = 0; i < tokens.size(); i++)
    {
        string& token = tokens[i];
        size_t pos = token.find(":");
        if (pos == string::npos || pos == 0 ||
            pos == token.length() - 1)
            continue;

        string name(token.substr(0, pos));
        string version(token.substr(pos + 1, token.length()));

        KComponentType type = MODULE;
        if (name == "sdk")
            type = SDK;
        else if (name == "mobilesdk")
            type = MOBILESDK;
        else if (name == "runtime")
            type = RUNTIME;

        jobs.push_back(Dependency::NewDependencyFromValues(
            type, name, version));
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
    int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::wstring applicationPath, updateFile;
    vector<SharedDependency> jobs;
    for (int i = 1; i < argc; i++)
    {
        wstring arg = argv[i];
        if (arg == L"-app")
        {
            i++;
            applicationPath = argv[i];
        }
        else if (arg == L"-update")
        {
            i++;
            updateFile = argv[i];
        }
        else
        {
            ParseCommandLineDependencies(argv[i], jobs);
        }
    }
    
    if (applicationPath.empty())
        ExitWithError("The installer could not determine the application path.", __LINE__);
 
    SharedApplication app(0);
    string utf8Path(WideToUTF8(applicationPath));
    if (updateFile.empty())
    {
        app = Application::NewApplication(utf8Path);
    }
    else
    {
        string utf8Update(WideToUTF8(updateFile));
        app = Application::NewApplication(utf8Update, utf8Path);

        //// Always delete the update file as soon as possible. That
        //// way the application can continue booting even if the update
        //// cannot be downloaded.
        FileUtils::DeleteFile(utf8Update);

        // If this is an application update, add a job for the update to
        // this list of necessary jobs.
        jobs.push_back(Dependency::NewDependencyFromValues(
            APP_UPDATE, "appupdate", app->version));
    }
 
    if (app.isNull())
        ExitWithError("The installer could not read the application manifest.", __LINE__);

    // This is a legacy action for Windows, but if the install file didn't
    // exist just write it out and finish up.
    string installFile(FileUtils::Join(app->GetDataPath().c_str(), ".installed", 0));
    bool installFileExisted = FileUtils::IsFile(installFile);
    if (!installFileExisted)
    {
        FileUtils::WriteFile(installFile, "");
    }

    if (jobs.empty() && installFileExisted)
        ExitWithError("The installer was not given any work to do.", __LINE__);

    if (CoInitialize(0) != S_OK)
        ExitWithError("The installer could not initialize COM.", __LINE__);

    dialog = new ProgressDialog();
    dialog->SetTitle(TideUtils::UTF8ToWide(app->name));
    dialog->SetCancelMessage(L"Aborting installation...");
    dialog->SetLineText(1, L"Installing components", false);
    dialog->Show();

    if (!dialog->GetWindowHandle())
        ExitWithError("Could not get progress dialog window handle.", __LINE__);

    bool success = HandleAllJobs(app, jobs);
    Cleanup();
    return success ? 0 : 1;
}
