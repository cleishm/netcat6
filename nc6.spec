Summary: Reads and writes data across network connections using TCP or UDP with IPv4 and IPv6.
Name: nc6
Version: 0.2pre3
Release: 1
Source0: ftp://ftp.ferrara.linux.it/pub/project6/sources/nc6-%{version}.tar.gz
Copyright: GPL
Group: Applications/Internet
BuildRoot: %{_tmppath}/%{name}-root

%description
The nc6 package contains Netcat6 (the program is actually nc6), a simple
utility for reading and writing data across network connections, using
the TCP or UDP protocols. Netcat6 is intended to be a reliable back-end
tool which can be used directly or easily driven by other programs and
scripts.  Netcat is also a feature-rich network debugging and
exploration tool, since it can create many different connections and
has many built-in capabilities.

You may want to install the netcat6 package if you are administering a
network and you'd like to use its debugging and network exploration
capabilities.

%prep
%setup

%build
#./bootstrap
./configure
make

%install
rm -rf ${RPM_BUILD_ROOT}
install -d ${RPM_BUILD_ROOT}%{_bindir}
install -m 755 src/nc6 ${RPM_BUILD_ROOT}%{_bindir}
install -d ${RPM_BUILD_ROOT}%{_mandir}/man1
install -m 644 docs/nc6.1 ${RPM_BUILD_ROOT}%{_mandir}/man1

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc README AUTHORS COPYING NEWS TODO ChangeLog
%{_bindir}/nc6
%{_mandir}/man1/nc6.1*

%changelog
* Sun Mar 10 2002 Peter Bieringer <pb@bieringer.de>
- Changes for nc6-0.2pre2, starting from nc-1.10-11 (RHL)

* Tue May 15 2001 Bill Nottingham <notting@redhat.com>
- add patch to fix timeouts (#40689, <jdp@ll.mit.edu>)

* Fri Oct 20 2000 Bill Nottingham <notting@redhat.com>
- include reslov.h for res_init prototype

* Fri Aug 11 2000 Jeff Johnson <jbj@redhat.com>
- add shutdown to fix obscure half-close connection case (#9324).

* Wed Jul 12 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Wed Jun 14 2000 Jeff Johnson <jbj@redhat.com>
- FHS packaging.

* Wed Feb 02 2000 Cristian Gafton <gafton@redhat.com>
- fix description
- add man page

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 4)

* Tue Jan 12 1999 Cristian Gafton <gafton@redhat.com>
- make it build on the arm

* Tue Dec 29 1998 Cristian Gafton <gafton@redhat.com>
- build for 6.0
