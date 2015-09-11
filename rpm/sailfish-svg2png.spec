Name:       sailfish-svg2png

Summary:    Sailfish SVG-2-PNG converter
Version:    0.1.3
Release:    1
Group:      System/Libraries
License:    Proprietary
URL:        https://bitbucket.org/jolla/ui-sailfish-svg2png
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  fdupes
Requires:   qt5-plugin-platform-minimal

%description
The Sailfish SVG-to-PNG converter.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5

%install
rm -rf %{buildroot}
%qmake5_install
%fdupes %{buildroot}/%{_libdir}

%files
%defattr(-,root,root,-)
%{_bindir}/sailfish_svg2png
%{_datadir}/qt5/mkspecs/features/sailfish-svg2png.prf


