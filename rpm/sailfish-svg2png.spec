Name:       sailfish-svg2png

Summary:    Sailfish SVG-2-PNG converter
Version:    0.3.2
Release:    1
License:    BSD
URL:        https://github.com/sailfishos/sailfish-svg2png
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(librsvg-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  fdupes
Requires:   qt5-plugin-platform-minimal

%description
The Sailfish SVG-to-PNG converter.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5

%install
%qmake5_install
%fdupes %{buildroot}/%{_libdir}

%files
%defattr(-,root,root,-)
%license LICENSE.BSD
%{_bindir}/sailfish_svg2png
%{_datadir}/qt5/mkspecs/features/sailfish-svg2png.prf
%{_datadir}/qt5/mkspecs/features/sailfish-svg2png-sizes.prf
%{_rpmmacrodir}/macros.sailfish-svg2png

