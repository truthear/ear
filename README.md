Bitnami Subversion Quick Start
==========================

 1. Default Sample Repository
 2. Subversion basic commands
 3. Subversion basic configuration
 4. More info

## 1 - Default Sample Repository
This is the README.md of the default Subversion repository created along with
Bitnami Subversion Stack.

The repository is located at `/opt/bitnami` and it has a first commit with
this README.md file by default. Due to the incremental nature of Subversion
repositories, you won't be able to see/modify the files of the repository
from that folder directly.

Its content is served by Apache at `http://your_ip/subversion`
Apache server will be listening by default at port 80 in Unix-like systems and
8080 in Windows.

## 2 - Subversion Basic Commands
In the command line of your server you can use the following commands to perform
actions with Subversion repositories.

#### Create an empty repository
    svnadmin create repo_folder

#### Checkout a repository
    svn checkout URL

#### Add a file to a repository
    svn add file

#### Commit changes to a repository
    svn commit -m "Message to commit" file

## 3 - How to enable commits over HTTP(s)
For security reasons, the default configuration does not allow users to commit
changes to the repositories over HTTP or HTTPS. To permit this, change the
permissions of your repository directory so that the Apache user is able to
write to it.

You can find more info to configure it at
[Bitnami Subversion Wiki](https://wiki.bitnami.com/Components/Subversion#How_to_enable_commits_over_HTTP%28S%29.3f)

## 4 - More info
You can find more info in the
[Bitnami Subversion Wiki Page](https://wiki.bitnami.com/Applications/BitNami_Subversion/)
or in the [official Subversion documentation](https://subversion.apache.org/docs/).

NOTE: If you are using a native installer you should replace the '/opt/bitnami'
with your installation directory.
