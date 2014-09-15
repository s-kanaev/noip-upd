# allow or disallow no-ip update filename (file has only $ALLOW_WORD word if update process is allowed)
ALLOW_UPD_F="/tmp/noip_upd.allow"
ALLOW_WORD="allow"
# disallow word
DISALLOW_WORD="disallow"

# time period to run no-ip update process
TIMEh=0
TIMEm=30
TIMEs=0

# path to noip_upd.bin binary executable
NOIP_UPD="/usr/local/bin/noip_upd.bin"

# path to noip_upd config file
NOIP_UPD_CONF="/usr/local/etc/noip_upd.conf"

# no executable word
NO_EXEC="No executable @ $NOIP_UPD"
