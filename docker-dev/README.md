Developing the Mud with Docker & Visual Studio Code
----------------

## Intro

This README was written by Faramir in May 2022 and it's aimed at people who are interested in using a Docker container to build, run and debug the mud. It was written with an emphasis on running the Docker container on a **Windows** box rather than Linux or Mac, although it should work fine there too, just ignore/substite mention of things like Power Shell for the *nix equivalent :)

The container layers a moderate number of tools on top of the *Ubuntu 22* base image. It's possible some important tools are missing, you're free to customize it as needed, and/or get in touch on Discord.

*Please be careful!* -- Don't put any important credentials into the image itself and publish the image to an untrusted image registry!

## Windows Setup

First, install WSL2. Follow the steps [here](https://docs.microsoft.com/en-us/windows/wsl/install). Make sure it's set to use Linux containers not Windows. 

Then install Docker Desktop for Windows. You can grab it [here](https://docs.docker.com/desktop/windows/release-notes/).

After that, you should be able to install a test image and start up a container to confirm Docker is working OK, e.g. from Power Shell:
```bash
docker pull hello-world
docker run --rm hello-world
```

## Install Visual Studio Code.

You can get it [here](https://code.visualstudio.com/download). Then install these VS Code extensions:
- Docker
- C/C++ Extension Pack
- C/C++ Intellisense Extension
- Clang-Tidy


## Build the Image

The image build steps create a user called `mudder`. This is the user you'll be running as when you enter the container.

The build relies on _Docker BuildKit_ to read a password variable and set it as the `mudder` password. The following commands include a step to set a password of your own choosing. Afterwards in the running container, use that password when using the `sudo` command to run things as `root`. You're free to change the password at any time with `passwd` of course.

Copy the `Dockerfile` from the sources into a local folder, such as `your_user/Documents/Docker`. Then in a Power Shell session run:

```bash
$Env:MUDPASS = "set_your_own"
$Env:DOCKER_BUILDKIT = 1

docker build --secret=id=mudpass,env=MUDPASS --progress=plain --rm -f Dockerfile -t mud:latest .
```

If all goes well, an image tagged as `mud:latest` should appear. You'll see it in the Docker Desktop app's Images panel, or you can list it with `docker image ls`.

## Create a Volume

The approach taken is to create a single persistent volume that is independent of the image itself and will be mounted on the `mudder` user's home directory path. That's where you will put the mud's git repo and make long-lived customizations.

```bash
docker volume create mudhut
```

## Launch the Container

There are a couple of ways to do this. 

### Launch from Terminal and Attach Docker Extension

This is my preferred way of doing it because it's really simple and being able to slam `ctrl-d` in a terminal to shut the container down has a certain appeal.

```bash
docker run --rm -it -v mudhut:/home/mudder mud:latest
```
You'll be in a `bash` terminal managed by a `tmux` session. You can spin up multiple tmux windows with `ctrl-b c`,  and switch between them with `ctrl-b n`. See more tmux shortcuts [here](https://tmuxcheatsheet.com/).


Now you can attach VS Code.  In its _Docker_ sidebar you will see `mud:latest` under the *CONTAINERS* section. Right click and _Attach Visual Studio Code_.  This will open a new VS Code window and the file navigator will be centered on `mudder`'s home directory. Plus VS Code will have a terminal panel ready for you to use.

From here you'll be able to do almost everything, including launch the mud's CPP Debug launch targets!


### Launch with VS Code Remote Containers

This is a bit more fiddly. As we want to mount the mudhut volume, it is necessary to add a config setting to tell it do that.

- F1, _Remote-Containers: Add Development Container Configuration Files..._
- Choose the option to pick an existing Dockerfile
- It'll open up a new `devcontainer.json` file.
- Customize it like so:
```
{
	"name": "Mud",
	"context": "..",
	"dockerFile": "../Dockerfile",
	"runArgs": [ "-v", "mudhut:/home/mudder" ]
}
```

More about that config [here](https://aka.ms/devcontainer.json).

- You should be able to F1 run _Remote-Containers: Open Folder in Container_ on the parent directory where `devcontainer.json` was written. 
- That directory will be mounted inside the container under `/workspace` and in VS Code's terminal pane, your session will start out in that directory. You'll probably want to cd to `mudder`'s home though and work there.

## Setting up the Container

There are now various setup tasks to consider doing, some being optional:

- You might want to run `sudo dpkg-reconfigure tzdata` to set your timezone.
- Generate fresh SSH keys and GPG signing keys, or copy existing ones over from another environment. That's not covered here.
- If your terminal colours are wonky try adding this to `~/.profile` -- `export TERM=xterm-256color`.
- Clone a fork of the mud's repo e.g.
```bash
mkdir dev
cd dev
git clone git@github.com:your-user/xania.git
```
- Kick off a build. Running the build will install various dependencies before compiling the apps and tests:
```bash
cd xania
make
git checkout -b my_branch # and let the hacking commence!
```
- The image includes the `aws` cli should you wish to use it for things on AWS. Just copy your configs & creds into the volume.


## Running Doorman & Xania

At this point you can run `doorman` and `xania`. There are alternative ways to do this depending on whether or not you want to use VS Code to debug. My personal preference is to do this:

- I start `doorman` from my bash/tmux session like so. Note that both apps look for several environment variables as mentioned in the top-level README. There are some reasonable defaults set in `mud-settings-dev.sh`. 
```bash
. mud-settings-dev.sh
./install/bin/doorman
```

- I start `xania` in debug mode in VS Code. From its _Run and Debug_ sidebar, the launch menu at the top will include several targets out of the box because there's a set of `.vscode` configs in the project's base directory, and VS Code will detect these automatically. Just use the one called _Debug Xania_.

Here's a screenshot of VS Code (2 windows, 1 connected to the container) and a tmux session in the container:

![Test](xania-doorman-vscode.PNG)