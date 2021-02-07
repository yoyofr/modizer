AC_INIT(Makefile.am)

#Variables
pwd=`pwd`
builders='${libdir}'/sidplay/builders
ac_configure_args="$ac_configure_args --disable-library-checks"

AC_CONFIG_AUX_DIR(unix)
AM_INIT_AUTOMAKE(sidplay-libs,2.1.1)
AC_CANONICAL_HOST

hardsid=false
case "$host" in
    *linux*) hardsid=true
        ;;
esac
AM_CONDITIONAL(HARDSID, test x$hardsid = xtrue)

echo; echo "Configuring libsidplay..."
cd $pwd/libsidplay
if ! eval ./configure $ac_configure_args \
--with-sidbuilders=$builders;
then
exit
fi

echo; echo "Configuring resid..."
cd "$pwd/resid"
if ! eval ./configure $ac_configure_args \
--disable-resid-install;
then
exit
fi

echo; echo "Configuring resid-builder..."
cd $pwd/builders/resid-builder
if ! eval ./configure $ac_configure_args \
--with-sidplay2-includes=$pwd/libsidplay/include \
--with-sidplay2-library=$pwd/libsidplay/src \
--with-resid-includes=$pwd/resid \
--with-resid-library=$pwd/resid \
--libdir=$builders;
then
exit
fi

if test x$hardsid = xtrue; then
    echo; echo "Configuring hardsid-builder..."
    cd $pwd/builders/hardsid-builder
    if ! eval ./configure $ac_configure_args \
        --with-sidplay2-includes=$pwd/libsidplay/include \
        --with-sidplay2-library=$pwd/libsidplay/src \
        --libdir=$builders;
    then
    exit
    fi
fi

echo; echo "Configuring libsidutils..."
cd $pwd/libsidutils
if ! eval ./configure $ac_configure_args \
--with-sidplay2-includes=$pwd/libsidplay/include \
--with-sidplay2-library=$pwd/libsidplay/src;
then
exit
fi

echo
cd $pwd

AC_OUTPUT(
Makefile \
builders/Makefile \
)
