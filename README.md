## libcurl, mbedTLS, and BSD sockets library for the Nintendo Wii

If you are just looking to install this into your dev environment, check GitHub releases
to the right and download the latest pkg files and install each of them with pacman.

If you would like to verify it worked correctly, you may clone this repo and build the sample application.
It should build with no warnings or errors, and it should run on a Wii (with a net connection) or Dolphin. The
application will use libcurl do an HTTPS HEAD request and an FTP get. The application will use mbedTLS to SHA-1
hash the file that was downloaded over FTP.


## LICENSE warning!

BSD Socket support is provided by a 3rd party library that is GPLv2 licensed. **You are responsible for determining
your obligations under the GPL for using any of these packages.** The makepkg and .patch files in this repo are not
necessarily under the GPL (or copyrightable at all, depending on jurisdiction), this is only referring to the
libraries procuded as a result that are statically linked into your code.


### Development

These take very little to maintain, I have release notifications for curl and I try to keep that as up to date
as possible. When updating curl I also check mbedTLS for updates. Socket support is by a 3rd party library and
is unmaintained.

curl requires no modifications to build, unless there is an upstream issue, if that happens I just search the
GitHub for the error and usually there is already a fix and I just bring in that commit as a .patch file.

mbedTLS has a small patch for hardware support, mainly a timer and a (fake) entropy source. This occasionally
needs a touchup after mbedTLS updates.
