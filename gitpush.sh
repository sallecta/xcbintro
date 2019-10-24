#!/bin/bash

#version 1.6
git add *
git add **/.gitignore
git add .gitignore
git add -u
echo 'Check status:' 
git status

if [ "$1" = "withCommit" ]
then
    commmitMessageArg="update gitpush script"
    echo "- press Enter to accept commit message \"$commmitMessageArg\" and upload (push);"
    echo '- type new non empty commit message and press Enter to upload (push);'
else
    echo "- press Enter to upload (push) with commit message \"...\""
    echo '- type commit message and press Enter to upload (push);'
fi

echo '- type Ctrl+C to cancel.'
read commmitMessageUsr

if [ "$commmitMessageUsr" = "" ]  && [ "$commmitMessageArg" = "" ]
then
    commmitMessage='...'
fi
if [ ! "$commmitMessageArg" = "" ]  && [ "$commmitMessageUsr" = "" ]
then
    commmitMessage=$commmitMessageArg
fi
if [ ! "$commmitMessageUsr" = "" ]
then
    commmitMessage=$commmitMessageUsr
fi

echo commit message is \"$commmitMessage\"

git commit --message="$commmitMessage"

git push

retVal=$?
if [ $retVal -ne 0 ]; then
    echo "git push error, reseting git commit"
    git reset --soft HEAD~1
fi
exit $retVal
