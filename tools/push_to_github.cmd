@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Push the current MycoLogger branch with ordinary Git safety checks.
rem Double-click this file for an interactive push, or pass --check to preview.

cd /d "%~dp0\.."

where git >nul 2>&1
if errorlevel 1 (
  echo ERROR: Git is not installed or is not available on PATH.
  goto :failed
)

git rev-parse --is-inside-work-tree >nul 2>&1
if errorlevel 1 (
  echo ERROR: Could not find the MycoLogger Git repository.
  goto :failed
)

for /f "delims=" %%B in ('git branch --show-current') do set "BRANCH=%%B"
if not defined BRANCH (
  echo ERROR: Git is in detached-HEAD state. Check out a branch before pushing.
  goto :failed
)

git remote get-url origin >nul 2>&1
if errorlevel 1 (
  echo ERROR: This repository has no Git remote named origin.
  goto :failed
)

for /f "delims=" %%U in ('git remote get-url origin') do set "REMOTE_URL=%%U"

echo.
echo MycoLogger GitHub push
echo ======================
echo Branch: %BRANCH%
echo Remote: %REMOTE_URL%
echo.
git status --short --branch
echo.

git rev-parse --verify "origin/%BRANCH%" >nul 2>&1
if errorlevel 1 (
  echo The remote branch origin/%BRANCH% does not exist yet.
  echo A successful push will create it and configure upstream tracking.
) else (
  for /f "delims=" %%C in ('git rev-list --count "origin/%BRANCH%..HEAD"') do set "AHEAD_COUNT=%%C"
  if "!AHEAD_COUNT!"=="0" (
    echo There are no local commits waiting to be pushed.
    goto :success
  )
  echo Commits waiting to be pushed: !AHEAD_COUNT!
  git log --oneline --decorate "origin/%BRANCH%..HEAD"
)

if /i "%~1"=="--check" (
  echo.
  echo Check complete. Nothing was pushed.
  goto :success
)

echo.
set "CONFIRM="
set /p "CONFIRM=Push these commits to origin/%BRANCH%? [y/N]: "
if /i not "%CONFIRM%"=="Y" (
  echo Push cancelled. Nothing was changed on GitHub.
  goto :success
)

echo.
git rev-parse --verify "origin/%BRANCH%" >nul 2>&1
if errorlevel 1 (
  git push --set-upstream origin "%BRANCH%"
) else (
  git push origin "%BRANCH%"
)
if errorlevel 1 (
  echo.
  echo ERROR: Git push failed. No force push was attempted.
  echo Read the Git message above, resolve the issue, and run this tool again.
  goto :failed
)

echo.
echo Push completed successfully.

:success
echo.
pause
exit /b 0

:failed
echo.
pause
exit /b 1
