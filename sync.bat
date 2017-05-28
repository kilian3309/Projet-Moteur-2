set /P msg="Message (can be void): "
"C:\Program Files\Git\bin\git.exe" commit -a --allow-empty-message -m '%msg%'
"C:\Program Files\Git\bin\git.exe" push
pause