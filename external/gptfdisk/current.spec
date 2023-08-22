Summary: GPT partitioning and MBR repair software
Name: gptfdisk
Version: 0.8.10

Release: 1%{?dist}
License: GPLv2
URL: http://www.rodsbooks.com/gdisk
Group: Applications/System
Source: http://www.rodsbooks.com/gdisk/gptfdisk-0.8.10.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description

Partitioning software for GPT disks and to repair MBR disks. The gdisk,
cgdisk, and sgdisk utilities (in the gdisk package) are GPT-enabled
partitioning tools; the fixparts utility (in the fixparts package) fixes
some problems with MBR disks that can be created by buggy partitioning
software.

%package -n gdisk

Group: Applications/System

Summary: An fdisk-like partitioning tool for GPT disks

%description -n gdisk
An fdisk-like partitioning tool for GPT disks. GPT
fdisk features a command-line interface, fairly direct
manipulation of partition table structures, recovery
tools to help you deal with corrupt partition tables,
and the ability to convert MBR disks to GPT format.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_CXX_FLAGS" make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/sbin
install -Dp -m0755 gdisk $RPM_BUILD_ROOT/usr/sbin
install -Dp -m0755 sgdisk $RPM_BUILD_ROOT/usr/sbin
install -Dp -m0755 cgdisk $RPM_BUILD_ROOT/usr/sbin
install -Dp -m0755 fixparts $RPM_BUILD_ROOT/usr/sbin
install -Dp -m0644 gdisk.8 $RPM_BUILD_ROOT/%{_mandir}/man8/gdisk.8
install -Dp -m0644 sgdisk.8 $RPM_BUILD_ROOT/%{_mandir}/man8/sgdisk.8
install -Dp -m0644 cgdisk.8 $RPM_BUILD_ROOT/%{_mandir}/man8/cgdisk.8
install -Dp -m0644 fixparts.8 $RPM_BUILD_ROOT/%{_mandir}/man8/fixparts.8

%clean
rm -rf $RPM_BUILD_ROOT

%files -n gdisk
%defattr(-,root,root -)
%doc NEWS COPYING README
/usr/sbin/gdisk
/usr/sbin/sgdisk
/usr/sbin/cgdisk
%doc %{_mandir}/man8/gdisk.8*
%doc %{_mandir}/man8/sgdisk.8*
%doc %{_mandir}/man8/cgdisk.8*

%package -n fixparts

Group: Applications/System

Summary: A tool for repairing certain types of damage to MBR disks

%description -n fixparts
A program that corrects errors that can creep into MBR-partitioned
disks. Removes stray GPT data, fixes mis-sized extended partitions,
and enables changing primary vs. logical partition status. Also
provides a few additional partition manipulation features.

%files -n fixparts
%defattr(-,root,root -)
%doc NEWS COPYING README
/usr/sbin/fixparts
%doc %{_mandir}/man8/fixparts.8*


%changelog
* Sun Mar 2 2014 R Smith <rodsmith@rodsbooks.com> - 0.8.10
- Created spec file for 0.8.10 release
