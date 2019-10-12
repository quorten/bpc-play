#! /bin/bash

OPENWRT="localhost"
SSH_CONTROL="/tmp/andrew_ssh_control"
SSH_CMD=(ssh -oControlMaster=no -oControlPath="$SSH_CONTROL" "$OPENWRT")

function cleanup() {
    set +e

    echo "Closing master SSH connection"
    "${SSH_CMD[@]}" -O stop

    echo "Removing temporary backup files"
    rm -r "$TMPDIR"
}
trap cleanup EXIT

# Open master ssh connection, to avoid the need to authenticate multiple times
echo "Opening master SSH connection"
ssh -oControlMaster=yes -oControlPath="$SSH_CONTROL" -o ControlPersist=1000 -n -N "$OPENWRT"

"${SSH_CMD[@]}" -t 'sudo -u asman -i' </dev/tty &

sleep 30

"${SSH_CMD[@]}" "echo hey, we\'re on the original"

sleep 30

# Clean up a little earlier, so the completion message is the last thing the user sees
cleanup
# Reset signal handler
trap EXIT

# Super ugly, but the solution is here.

# Sync backup to a user that cannot ssh directly to, but can access
# through sudo?

# A few methods.

# Unfortunately, most of these techniques will commonly render the
# password visible on the command line.

# 1. `sudo -S' to read password from standard input.  This works
#    with rsync too.
# rsync -R -avz -e ssh --rsync-path="echo mypassword | sudo -S  mkdir -p /remote/lovely/folder && sudo rsync" ...
# 20180721/https://superuser.com/questions/270911/run-rsync-with-root-permission-on-remote-machine

# 2. `sudo` NOPASSWD.  Unfortunately this will open up new attack
# vectors on a compromised system.  This works with rsync too:
# rsync ... --rsync-path="sudo rsync" ...
# 20180721/https://askubuntu.com/questions/719439/using-rsync-with-sudo-on-the-destination-machine
# 20180721/https://www.simplified.guide/ssh/sudo-no-tty-askpass
# 20180721/https://www.lullabot.com/articles/simple-offsite-backups-with-rsync-ssh-and-sudo

# 3. `expect' to channel commands through a single TTY on the remote.
# Perhaps it should be evident now that `sudo' is the bottleneck, at
# least in default Ubuntu-style configuration.  But, the point here is
# that if we use `expect', we can control the pseudo-terminal and
# channel all commands through a single coherent terminal.  This way,
# `sudo' will see that the timestamp on the terminal is the same, and
# won't ask for a password on subsequent commands executed within the
# same time frame.  Apparently this can also be done with rsync too...
# if you use the rsync shell execute commands as above.  And you hack
# the remote shell used.

# 4. Rather than requiring a full `expect' implementation, we write
# our own wrapper program to step around the sudo problem, and then
# use all of our tricks with that instead.
