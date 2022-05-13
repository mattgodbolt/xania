# A Dockerfile that creates a fairly cut down Ubuntu 22 image that can be
# used to build, run and debug the mud and its related tools. 
# See README.md in this directory.
FROM ubuntu:latest
RUN \
 echo 'Acquire::http::Timeout "5";\nAcquire::ftp::Timeout "5";\n;' > /etc/apt/apt.conf.d/99timeout && \
 apt-get -qq update && \
 DEBIAN_FRONTEND="noninteractive" apt-get -y --no-install-recommends install  \
 dialog tzdata sudo locales tmux  wget curl gnupg git-core nano iputils-ping ssh \
 build-essential software-properties-common cmake ninja-build telnet rlwrap awscli \
 gdb less

RUN locale-gen en_US.UTF-8 && \
    adduser \
        --quiet \
        --disabled-password \
        --shell /usr/bin/bash \
        --home /home/mudder \
        --gecos "Mudder" mudder && \
    usermod -aG sudo mudder && \
    echo 'export GREP_COLORS="fn=37"' > /etc/profile.d/grep.sh && \
    echo 'export GPG_TTY=$(tty)' > /etc/profile.d/gpgtty.sh

RUN --mount=type=secret,id=mudpass \
    echo "mudder:`cat /run/secrets/mudpass`" | chpasswd

USER mudder
ENV TERM xterm
ENV LOCALE=en_US-UTF8
WORKDIR /home/mudder
ENTRYPOINT ["tmux"]
