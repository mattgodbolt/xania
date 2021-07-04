/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

template <typename F, typename B>
inline bool check_bit(const F flag, const B bit) {
    return flag & bit;
}

template <typename F, typename B>
inline bool set_bit(F &flag, const B bit) {
    return flag |= bit;
}

template <typename F, typename B>
inline bool clear_bit(F &flag, const B bit) {
    return flag &= (~bit);
}
