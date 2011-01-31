# $Id$
# $URL$

Summary: Server extensions for the MBT tagger
Name: mbtserver
Version: 0.1
Release: 1
License: GPL
Group: Applications/System
URL: http://ilk.uvt.nl/mbt/

Packager: Joost van Baal <joostvb-mbt@ad1810.com>
Vendor: ILK, http://ilk.uvt.nl/

# rpm and src.rpm should end up in
# zeus:/var/www/ilk/packages.
# orig source available from
# zeus:/var/www/ilk/downloads/pub/software/mbt-3.2.2.tar.gz
Source: http://ilk.uvt.nl/downloads/pub/software/mbtserver-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

Requires: timbl, timblserver, mbt
BuildRequires: gcc-c++, timbl, timblserver, mbt

%description
MbtServer extends Mbt with a server layer, running as a TCP server.  Mbt is a
memory-based tagger-generator and tagger for natural language processing.
MbtServer provides the possibility to access a trained tagger from multiple
sessions. It also allows to run and access different taggers in parallel.

MbtServer is a product of the ILK Research Group (Tilburg University, The
Netherlands) and the CLiPS Research Centre (University of Antwerp, Belgium).

If you do scientific research in natural language processing, MbtServer will
likely be of use to you.

%prep
%setup

%build
%configure

%install
%{__rm} -rf %{buildroot}
%makeinstall

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 0755)
%doc AUTHORS ChangeLog NEWS README TODO
%{_datadir}/doc/%{name}/examples/*
%{_bindir}/MbtServer
%{_includedir}/%{name}/*.h
%{_mandir}/man*/MbtServer*

%changelog
* Mon Jan 31 2011 Joost van Baal <joostvb-timbl@ad1810.com> - 0.1-1
- Initial release.

