#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include <algorithm>
#include <array>
#include <commctrl.h>
#include <cstring>
#include <cwctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>


#define ID_COMBO_TYPE 40100
#define ID_COMBO_BASE 40101
#define ID_BTN_A 41000
#define ID_BTN_B 41001
#define ID_BTN_C 41002
#define ID_BTN_D 41003
#define ID_BTN_E 41004
#define ID_BTN_F 41005
#define ID_BTN_7 41006
#define ID_BTN_8 41007
#define ID_BTN_9 41008
#define ID_BTN_DIV 41009
#define ID_BTN_BS 41010
#define ID_BTN_CLEAR 41011
#define ID_BTN_4 41012
#define ID_BTN_5 41013
#define ID_BTN_6 41014
#define ID_BTN_MUL 41015
#define ID_BTN_SUB 41016
#define ID_BTN_ADD 41017
#define ID_BTN_1 41018
#define ID_BTN_2 41019
#define ID_BTN_3 41020
#define ID_BTN_0 41021
#define ID_BTN_DOT 41022
#define ID_BTN_EQ 41023

namespace {

enum class AppMode { Basic, Programmer };
enum class NumberBase { Hex, Dec, Oct, Bin };
enum class DataType {
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float16,
    Float32,
    Float64
};

struct TypeMeta {
    const wchar_t *name;
    int bits, exp, mant, bias;
    bool is_int, is_signed;
};
constexpr std::array<TypeMeta, 11> kTypeMeta{
    {{L"int8", 8, 0, 0, 0, true, true},
     {L"uint8", 8, 0, 0, 0, true, false},
     {L"int16", 16, 0, 0, 0, true, true},
     {L"uint16", 16, 0, 0, 0, true, false},
     {L"int32", 32, 0, 0, 0, true, true},
     {L"uint32", 32, 0, 0, 0, true, false},
     {L"int64", 64, 0, 0, 0, true, true},
     {L"uint64", 64, 0, 0, 0, true, false},
     {L"float16", 16, 5, 10, 15, false, false},
     {L"float32", 32, 8, 23, 127, false, false},
     {L"float64", 64, 11, 52, 1023, false, false}}};
constexpr std::array<const wchar_t *, 4> kBaseNames{L"Hex", L"Dec", L"Oct",
                                                    L"Bin"};
constexpr BYTE kInactiveAlpha = 178;

template <typename E>
    requires std::is_enum_v<E>
constexpr int idx(E e) {
    return static_cast<int>(e);
}
const TypeMeta &meta(DataType t) { return kTypeMeta[idx(t)]; }
int bit_width(DataType t) { return meta(t).bits; }
bool is_integer_type(DataType t) { return meta(t).is_int; }
bool is_float_type(DataType t) { return !meta(t).is_int; }
bool is_signed_integer_type(DataType t) { return meta(t).is_signed; }
const wchar_t *type_name(DataType t) { return meta(t).name; }
const wchar_t *base_name(NumberBase b) { return kBaseNames[idx(b)]; }
uint64_t mask_for_bits(int bits) {
    return bits >= 64 ? ~0ull : ((1ull << bits) - 1);
}

struct DisplayState {
    std::wstring history, current = L"0", warning;
    HFONT hist = nullptr, main = nullptr, warn = nullptr;
};
struct RuntimeButton {
    int id;
    const wchar_t *text;
    HWND hwnd = nullptr;
};
struct ProgrammerState {
    DataType type = DataType::Int32;
    NumberBase base = NumberBase::Dec;
    uint64_t current_raw = 0, stored_raw = 0;
    wchar_t pending_op = 0;
    bool reset_input = false, just_evaluated = false, has_exact_input = false;
    int hovered_bit = -1;
    HWND hTypeCombo = nullptr, hBaseCombo = nullptr, hBitDisplay = nullptr;
    std::wstring history, exact_input, precision_warning, tooltip_text;
};
struct AppConfig {
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 900, h = 700;
    AppMode mode = AppMode::Basic;
    DataType type = DataType::Int32;
    NumberBase base = NumberBase::Dec;
    bool separators = true, topmost = false;
};

constexpr std::array<RuntimeButton, 24> kButtons{
    {{ID_BTN_A, L"A"},     {ID_BTN_B, L"B"},   {ID_BTN_C, L"C"},
     {ID_BTN_D, L"D"},     {ID_BTN_E, L"E"},   {ID_BTN_F, L"F"},
     {ID_BTN_7, L"7"},     {ID_BTN_8, L"8"},   {ID_BTN_9, L"9"},
     {ID_BTN_DIV, L"/"},   {ID_BTN_4, L"4"},   {ID_BTN_5, L"5"},
     {ID_BTN_6, L"6"},     {ID_BTN_MUL, L"*"}, {ID_BTN_1, L"1"},
     {ID_BTN_2, L"2"},     {ID_BTN_3, L"3"},   {ID_BTN_SUB, L"-"},
     {ID_BTN_CLEAR, L"C"}, {ID_BTN_0, L"0"},   {ID_BTN_DOT, L"."},
     {ID_BTN_ADD, L"+"},   {ID_BTN_BS, L"BS"}, {ID_BTN_EQ, L"="}}};
constexpr std::array<int, 18> kBasicLayout{
    {ID_BTN_7, ID_BTN_8, ID_BTN_9, ID_BTN_DIV, ID_BTN_4, ID_BTN_5, ID_BTN_6,
     ID_BTN_MUL, ID_BTN_1, ID_BTN_2, ID_BTN_3, ID_BTN_SUB, ID_BTN_CLEAR,
     ID_BTN_0, ID_BTN_DOT, ID_BTN_ADD, ID_BTN_BS, ID_BTN_EQ}};
constexpr std::array<int, 6> kLetterButtons{
    {ID_BTN_A, ID_BTN_B, ID_BTN_C, ID_BTN_D, ID_BTN_E, ID_BTN_F}};
constexpr std::array<std::pair<int, int>, 10> kDigitButtons = {{{ID_BTN_9, 9},
                                                                {ID_BTN_8, 8},
                                                                {ID_BTN_7, 7},
                                                                {ID_BTN_6, 6},
                                                                {ID_BTN_5, 5},
                                                                {ID_BTN_4, 4},
                                                                {ID_BTN_3, 3},
                                                                {ID_BTN_2, 2},
                                                                {ID_BTN_1, 1},
                                                                {ID_BTN_0, 0}}};

HINSTANCE g_instance = nullptr;
HWND g_window_handle = nullptr, g_display_handle = nullptr;
DisplayState g_display;
ProgrammerState g_programmer;
std::vector<RuntimeButton> g_buttons;
AppMode g_mode = AppMode::Basic;
bool g_is_topmost = false, g_use_separators = true;
std::wstring g_ini_path, g_current_input = L"0", g_history;
AppConfig g_initial_config;
bool g_reset_input = false, g_just_evaluated = false;
double g_stored_value = 0.0;
wchar_t g_pending_op = 0;

void update_buttons_enabled(), update_menu_checks(), sync_display_from_state(),
    layout_children(HWND);

HWND find_button(int id) {
    for (auto &b : g_buttons)
        if (b.id == id)
            return b.hwnd;
    return nullptr;
}
std::wstring trim(const std::wstring &s) {
    auto a = s.find_first_not_of(L" \t\r\n"),
         b = s.find_last_not_of(L" \t\r\n");
    return a == std::wstring::npos ? L"" : s.substr(a, b - a + 1);
}

std::wstring group_digits_every_four(std::wstring s) {
    if (!g_use_separators || s.size() <= 4)
        return s;
    std::wstring out;
    size_t first = s.size() % 4;
    if (!first)
        first = 4;
    out.append(s, 0, first);
    for (size_t i = first; i < s.size(); i += 4)
        out += L' ', out.append(s, i, 4);
    return out;
}

wchar_t digit_from_button_id(int id) {
    switch (id) {
    case ID_BTN_0:
        return L'0';
    case ID_BTN_1:
        return L'1';
    case ID_BTN_2:
        return L'2';
    case ID_BTN_3:
        return L'3';
    case ID_BTN_4:
        return L'4';
    case ID_BTN_5:
        return L'5';
    case ID_BTN_6:
        return L'6';
    case ID_BTN_7:
        return L'7';
    case ID_BTN_8:
        return L'8';
    case ID_BTN_9:
        return L'9';
    default:
        return 0;
    }
}
wchar_t hex_from_button_id(int id) {
    return id >= ID_BTN_A && id <= ID_BTN_F ? L'A' + (id - ID_BTN_A) : 0;
}
wchar_t op_from_button_id(int id) {
    switch (id) {
    case ID_BTN_ADD:
        return L'+';
    case ID_BTN_SUB:
        return L'-';
    case ID_BTN_MUL:
        return L'*';
    case ID_BTN_DIV:
        return L'/';
    default:
        return 0;
    }
}

void invalidate_ui() {
    if (g_display_handle)
        InvalidateRect(g_display_handle, nullptr, TRUE);
    if (g_mode == AppMode::Programmer && g_programmer.hBitDisplay)
        InvalidateRect(g_programmer.hBitDisplay, nullptr, TRUE);
}

template <class T>
bool try_parse_number(const std::wstring &s, int base, T &out) {
    try {
        size_t p = 0;
        if constexpr (std::is_same_v<T, double>)
            out = std::stod(s, &p);
        else if constexpr (std::is_same_v<T, long double>)
            out = std::stold(s, &p);
        else if constexpr (std::is_same_v<T, uint64_t>) {
            if (s.empty())
                return false;
            out = std::stoull(s, &p, base);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            if (s.empty())
                return false;
            out = std::stoll(s, &p, base);
        } else
            return false;
        return p == s.size();
    } catch (...) {
        return false;
    }
}

template <class To, class From> To bit_copy(const From &v) {
    static_assert(sizeof(To) == sizeof(From));
    To out{};
    std::memcpy(&out, &v, sizeof(To));
    return out;
}

uint16_t float_to_half_bits(float v) {
    uint32_t b = bit_copy<uint32_t>(v), s = (b >> 31) & 1, e = (b >> 23) & 0xFF,
             m = b & 0x7FFFFF;
    if (e == 255)
        return static_cast<uint16_t>((s << 15) | (m ? 0x7E00 : 0x7C00));
    int he = static_cast<int>(e) - 127 + 15;
    if (he >= 31)
        return static_cast<uint16_t>((s << 15) | 0x7C00);
    if (he <= 0) {
        if (he < -10)
            return static_cast<uint16_t>(s << 15);
        uint32_t mm = m | 0x800000;
        int shift = 14 - he;
        uint16_t hm = static_cast<uint16_t>(mm >> shift);
        if ((mm >> (shift - 1)) & 1u)
            ++hm;
        return static_cast<uint16_t>((s << 15) | hm);
    }
    uint16_t hm = static_cast<uint16_t>(m >> 13);
    if (m & 0x1000)
        if (++hm == 0x400)
            return static_cast<uint16_t>((s << 15) | ((he + 1) << 10));
    return static_cast<uint16_t>((s << 15) | (he << 10) | hm);
}

float half_bits_to_float(uint16_t h) {
    uint32_t s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF, b = 0;
    if (!e) {
        if (!m)
            b = s << 31;
        else {
            int ee = -14;
            while ((m & 0x400) == 0)
                m <<= 1, --ee;
            m &= 0x3FF;
            b = (s << 31) | ((ee + 127) << 23) | (m << 13);
        }
    } else if (e == 0x1F)
        b = (s << 31) | 0x7F800000 | (m << 13);
    else
        b = (s << 31) | ((e - 15 + 127) << 23) | (m << 13);
    return bit_copy<float>(b);
}

long double raw_to_long_double(uint64_t raw, DataType t) {
    raw &= mask_for_bits(bit_width(t));
    switch (t) {
    case DataType::Int8:
        return static_cast<long double>(static_cast<int8_t>(raw));
    case DataType::UInt8:
        return static_cast<long double>(static_cast<uint8_t>(raw));
    case DataType::Int16:
        return static_cast<long double>(static_cast<int16_t>(raw));
    case DataType::UInt16:
        return static_cast<long double>(static_cast<uint16_t>(raw));
    case DataType::Int32:
        return static_cast<long double>(static_cast<int32_t>(raw));
    case DataType::UInt32:
        return static_cast<long double>(static_cast<uint32_t>(raw));
    case DataType::Int64:
        return static_cast<long double>(static_cast<int64_t>(raw));
    case DataType::UInt64:
        return static_cast<long double>(raw);
    case DataType::Float16:
        return static_cast<long double>(
            half_bits_to_float(static_cast<uint16_t>(raw)));
    case DataType::Float32:
        return static_cast<long double>(
            bit_copy<float>(static_cast<uint32_t>(raw)));
    case DataType::Float64:
        return static_cast<long double>(bit_copy<double>(raw));
    }
    return 0;
}

uint64_t cast_long_double_to_raw(long double v, DataType t) {
    switch (t) {
    case DataType::Int8:
        return static_cast<uint8_t>(static_cast<int8_t>(v));
    case DataType::UInt8:
        return static_cast<uint8_t>(v);
    case DataType::Int16:
        return static_cast<uint16_t>(static_cast<int16_t>(v));
    case DataType::UInt16:
        return static_cast<uint16_t>(v);
    case DataType::Int32:
        return static_cast<uint32_t>(static_cast<int32_t>(v));
    case DataType::UInt32:
        return static_cast<uint32_t>(v);
    case DataType::Int64:
        return static_cast<uint64_t>(static_cast<int64_t>(v));
    case DataType::UInt64:
        return static_cast<uint64_t>(v);
    case DataType::Float16:
        return float_to_half_bits(static_cast<float>(v));
    case DataType::Float32:
        return bit_copy<uint32_t>(static_cast<float>(v));
    case DataType::Float64:
        return bit_copy<uint64_t>(static_cast<double>(v));
    }
    return 0;
}

int64_t sign_extend_to_int64(uint64_t raw, int bits) {
    if (bits >= 64)
        return static_cast<int64_t>(raw);
    uint64_t mask = mask_for_bits(bits);
    raw &= mask;
    if (raw & (1ull << (bits - 1)))
        raw |= ~mask;
    return static_cast<int64_t>(raw);
}

std::wstring trim_trailing_zeros_fixed(double v) {
    std::wostringstream os;
    os << std::setprecision(15) << std::fixed << v;
    auto s = os.str();
    auto p = s.find_last_not_of(L'0');
    if (p == std::wstring::npos)
        return L"0";
    s.erase(p + 1);
    if (!s.empty() && s.back() == L'.')
        s.pop_back();
    return s.empty() ? L"0" : s;
}

std::wstring format_float(long double v, int digits) {
    std::wostringstream os;
    os << std::setprecision(digits) << std::defaultfloat << v;
    auto s = os.str();
    if (auto e = s.find_first_of(L"eE"); e != std::wstring::npos) {
        auto mant = s.substr(0, e), expo = s.substr(e);
        if (mant.find(L'.') != std::wstring::npos) {
            while (!mant.empty() && mant.back() == L'0')
                mant.pop_back();
            if (!mant.empty() && mant.back() == L'.')
                mant.pop_back();
        }
        return mant + expo;
    }
    if (s.find(L'.') != std::wstring::npos) {
        while (!s.empty() && s.back() == L'0')
            s.pop_back();
        if (!s.empty() && s.back() == L'.')
            s.pop_back();
    }
    return s.empty() ? L"0" : s;
}

std::wstring format_unsigned_base(uint64_t raw, NumberBase base, int bits,
                                  bool pad) {
    raw &= mask_for_bits(bits);
    int radix = 10;
    std::wstring prefix;
    switch (base) {
    case NumberBase::Hex:
        radix = 16;
        prefix = L"0x";
        break;
    case NumberBase::Oct:
        radix = 8;
        prefix = L"o";
        break;
    case NumberBase::Bin:
        radix = 2;
        prefix = L"b";
        break;
    default:
        break;
    }
    if (radix == 10)
        return std::to_wstring(raw);
    if (!raw && !pad)
        return prefix + L"0";
    static constexpr wchar_t d[] = L"0123456789ABCDEF";
    std::wstring s;
    do
        s.push_back(d[raw % radix]), raw /= radix;
    while (raw);
    std::reverse(s.begin(), s.end());
    if (pad) {
        int w = base == NumberBase::Hex   ? (bits + 3) / 4
                : base == NumberBase::Oct ? (bits + 2) / 3
                                          : bits;
        if ((int)s.size() < w)
            s.insert(s.begin(), w - (int)s.size(), L'0');
    }
    if (base == NumberBase::Hex || base == NumberBase::Bin)
        s = group_digits_every_four(s);
    return prefix + s;
}

std::wstring format_programmer_value(uint64_t raw, DataType t, NumberBase b) {
    int bits = bit_width(t);
    raw &= mask_for_bits(bits);
    if (b != NumberBase::Dec)
        return format_unsigned_base(raw, b, bits, true);
    if (is_integer_type(t))
        return is_signed_integer_type(t)
                   ? std::to_wstring(sign_extend_to_int64(raw, bits))
                   : std::to_wstring(static_cast<unsigned long long>(raw));
    int digits = t == DataType::Float16 ? 6
                 : t == DataType::Float32
                     ? 9
                     : std::numeric_limits<double>::max_digits10;
    return format_float(raw_to_long_double(raw, t), digits);
}

bool dot_allowed_for_programmer() {
    return g_programmer.base == NumberBase::Dec &&
           is_float_type(g_programmer.type);
}
void sync_basic_display_from_state() {
    g_display.history = g_history;
    g_display.current = g_current_input.empty() ? L"0" : g_current_input;
    g_display.warning.clear();
    invalidate_ui();
}
void sync_programmer_display_from_state() {
    g_display.history = g_programmer.history;
    if (!g_programmer.has_exact_input)
        g_display.current = format_programmer_value(g_programmer.current_raw,
                                                    g_programmer.type,
                                                    g_programmer.base);
    else if (g_programmer.base == NumberBase::Hex)
        g_display.current = L"0x" + g_programmer.exact_input;
    else if (g_programmer.base == NumberBase::Oct)
        g_display.current = L"o" + g_programmer.exact_input;
    else if (g_programmer.base == NumberBase::Bin)
        g_display.current = L"b" + g_programmer.exact_input;
    else
        g_display.current =
            g_programmer.exact_input.empty() ? L"0" : g_programmer.exact_input;
    g_display.warning = g_programmer.precision_warning;
    invalidate_ui();
}
void sync_display_from_state() {
    (g_mode == AppMode::Basic ? sync_basic_display_from_state
                              : sync_programmer_display_from_state)();
}

void clear_basic_state() {
    g_current_input = L"0";
    g_history.clear();
    g_stored_value = 0;
    g_pending_op = 0;
    g_reset_input = g_just_evaluated = false;
}
void clear_programmer_state() {
    g_programmer.current_raw = g_programmer.stored_raw = 0;
    g_programmer.pending_op = 0;
    g_programmer.history.clear();
    g_programmer.reset_input = g_programmer.just_evaluated =
        g_programmer.has_exact_input = false;
    g_programmer.hovered_bit = -1;
    g_programmer.exact_input.clear();
    g_programmer.precision_warning.clear();
    g_programmer.tooltip_text.clear();
}
void clear_all() {
    (g_mode == AppMode::Basic ? clear_basic_state : clear_programmer_state)();
    sync_display_from_state();
}

bool apply_pending_basic_operation() {
    double rhs = 0;
    if (!try_parse_number(g_current_input, 10, rhs))
        return MessageBoxW(g_window_handle, L"Invalid number.", L"Error",
                           MB_ICONERROR),
               false;
    switch (g_pending_op) {
    case 0:
        g_stored_value = rhs;
        break;
    case L'+':
        g_stored_value += rhs;
        break;
    case L'-':
        g_stored_value -= rhs;
        break;
    case L'*':
        g_stored_value *= rhs;
        break;
    case L'/':
        g_stored_value = rhs ? g_stored_value / rhs : 0.0;
        break;
    default:
        return false;
    }
    return true;
}

void basic_input_digit(wchar_t ch) {
    g_current_input = (g_reset_input || g_current_input == L"0")
                          ? std::wstring(1, ch)
                          : g_current_input + ch;
    g_reset_input = g_just_evaluated = false;
    sync_basic_display_from_state();
}
void basic_input_dot() {
    if (g_reset_input)
        g_current_input = L"0.", g_reset_input = false;
    else if (g_current_input.find(L'.') == std::wstring::npos)
        g_current_input += L'.';
    g_just_evaluated = false;
    sync_basic_display_from_state();
}
void basic_backspace() {
    if (g_reset_input || g_just_evaluated)
        return;
    if (!g_current_input.empty())
        g_current_input.pop_back();
    if (g_current_input.empty() || g_current_input == L"-")
        g_current_input = L"0";
    sync_basic_display_from_state();
}
void basic_input_operator(wchar_t op) {
    if (!apply_pending_basic_operation())
        return;
    auto v = trim_trailing_zeros_fixed(g_stored_value);
    g_history = v + L" " + std::wstring(1, op);
    g_pending_op = op;
    g_current_input = v;
    g_reset_input = true;
    g_just_evaluated = false;
    sync_basic_display_from_state();
}
void basic_evaluate() {
    if (!apply_pending_basic_operation())
        return;
    g_history = g_pending_op ? g_history + L" " + g_current_input + L" ="
                             : g_current_input + L" =";
    g_current_input = trim_trailing_zeros_fixed(g_stored_value);
    g_pending_op = 0;
    g_reset_input = g_just_evaluated = true;
    sync_basic_display_from_state();
}

void update_precision_warning() {
    g_programmer.precision_warning.clear();
    if (!(is_float_type(g_programmer.type) &&
          g_programmer.base == NumberBase::Dec && g_programmer.has_exact_input))
        return;
    long double typed = 0;
    if (!try_parse_number(g_programmer.exact_input, 10, typed))
        return;
    if (typed !=
        raw_to_long_double(g_programmer.current_raw, g_programmer.type))
        g_programmer.precision_warning =
            L"stored: " + format_programmer_value(g_programmer.current_raw,
                                                  g_programmer.type,
                                                  NumberBase::Dec);
}

bool parse_programmer_input_to_raw(std::wstring text, uint64_t &out) {
    auto first_non_ws = text.find_first_not_of(L" \t\r\n");
    if (first_non_ws == std::wstring::npos)
        text.clear();
    else {
        auto last_non_ws = text.find_last_not_of(L" \t\r\n");
        text = text.substr(first_non_ws, last_non_ws - first_non_ws + 1);
    }
    if (text.empty() || text == L"-" || text == L"+" || text == L".")
        return out = 0, true;
    if (g_programmer.base == NumberBase::Dec) {
        int bits = bit_width(g_programmer.type);
        if (is_integer_type(g_programmer.type)) {
            if (is_signed_integer_type(g_programmer.type)) {
                int64_t v = 0;
                return try_parse_number(text, 10, v)
                           ? (out = static_cast<uint64_t>(v) &
                                    mask_for_bits(bits),
                              true)
                           : false;
            }
            uint64_t v = 0;
            return try_parse_number(text, 10, v)
                       ? (out = v & mask_for_bits(bits), true)
                       : false;
        }
        long double v = 0;
        return try_parse_number(text, 10, v)
                   ? (out = cast_long_double_to_raw(v, g_programmer.type) &
                            mask_for_bits(bits),
                      true)
                   : false;
    }
    int base = g_programmer.base == NumberBase::Hex   ? 16
               : g_programmer.base == NumberBase::Oct ? 8
                                                      : 2;
    text.erase(
        std::remove_if(text.begin(), text.end(),
                       [](wchar_t c) { return c == L' ' || c == L'\t'; }),
        text.end());
    uint64_t v = 0;
    return try_parse_number(text, base, v)
               ? (out = v & mask_for_bits(bit_width(g_programmer.type)), true)
               : false;
}

void commit_exact_programmer_input(bool showError) {
    if (!g_programmer.has_exact_input)
        return update_precision_warning();
    uint64_t raw = 0;
    if (!parse_programmer_input_to_raw(g_programmer.exact_input, raw)) {
        if (showError)
            MessageBoxW(g_window_handle,
                        L"Invalid value for the selected type/base.", L"Error",
                        MB_ICONERROR);
        return;
    }
    g_programmer.current_raw =
        raw & mask_for_bits(bit_width(g_programmer.type));
    update_precision_warning();
}

void clear_programmer_exact_input() {
    g_programmer.exact_input.clear();
    g_programmer.has_exact_input = false;
    g_programmer.precision_warning.clear();
}
void programmer_begin_fresh_input_if_needed() {
    if (g_programmer.reset_input)
        clear_programmer_exact_input(), g_programmer.reset_input = false;
}

void programmer_input_char(wchar_t ch) {
    ch = static_cast<wchar_t>(towupper(ch));
    programmer_begin_fresh_input_if_needed();
    if (!g_programmer.has_exact_input)
        g_programmer.has_exact_input = true;
    auto &s = g_programmer.exact_input;
    if (s == L"0" && ch != L'0' && g_programmer.base != NumberBase::Dec)
        s.clear();
    if (g_programmer.base == NumberBase::Dec)
        s = (s == L"0" && ch != L'0' && s.find(L'.') == std::wstring::npos)
                ? std::wstring(1, ch)
                : s + ch;
    else {
        if (s == L"0")
            s.clear();
        s += ch;
    }
    g_programmer.just_evaluated = false;
    commit_exact_programmer_input(false);
    sync_programmer_display_from_state();
}

void programmer_input_dot() {
    if (!dot_allowed_for_programmer())
        return;
    programmer_begin_fresh_input_if_needed();
    auto &s = g_programmer.exact_input;
    g_programmer.has_exact_input = true;
    if (s.empty())
        s = L"0.";
    else if (s == L"-")
        s += L"0.";
    else if (s.find(L'.') == std::wstring::npos)
        s += L'.';
    g_programmer.just_evaluated = false;
    commit_exact_programmer_input(false);
    sync_programmer_display_from_state();
}

void programmer_toggle_sign() {
    if (g_programmer.base != NumberBase::Dec)
        return;
    programmer_begin_fresh_input_if_needed();
    if (!g_programmer.has_exact_input)
        g_programmer.exact_input = format_programmer_value(
            g_programmer.current_raw, g_programmer.type, NumberBase::Dec),
        g_programmer.has_exact_input = true;
    auto &s = g_programmer.exact_input;
    if (!s.empty() && s.front() == L'-')
        s.erase(s.begin());
    else
        s.insert(s.begin(), L'-');
    commit_exact_programmer_input(false);
    sync_programmer_display_from_state();
}

void programmer_backspace() {
    if (g_programmer.reset_input || g_programmer.just_evaluated)
        return;
    if (g_programmer.has_exact_input) {
        if (!g_programmer.exact_input.empty())
            g_programmer.exact_input.pop_back();
        if (g_programmer.exact_input.empty() ||
            g_programmer.exact_input == L"-")
            g_programmer.exact_input = L"0";
        commit_exact_programmer_input(false);
    } else
        g_programmer.current_raw = (g_programmer.current_raw >> 1) &
                                   mask_for_bits(bit_width(g_programmer.type));
    sync_programmer_display_from_state();
}

bool apply_pending_programmer_operation() {
    commit_exact_programmer_input(true);
    if (!g_programmer.pending_op)
        return g_programmer.stored_raw = g_programmer.current_raw,
               clear_programmer_exact_input(), true;
    int bits = bit_width(g_programmer.type);
    uint64_t mask = mask_for_bits(bits);
    if (is_integer_type(g_programmer.type)) {
        if (is_signed_integer_type(g_programmer.type)) {
            int64_t a = sign_extend_to_int64(g_programmer.stored_raw, bits),
                    b = sign_extend_to_int64(g_programmer.current_raw, bits),
                    r = 0;
            switch (g_programmer.pending_op) {
            case L'+':
                r = a + b;
                break;
            case L'-':
                r = a - b;
                break;
            case L'*':
                r = a * b;
                break;
            case L'/':
                r = b ? a / b : 0;
                break;
            default:
                return false;
            }
            g_programmer.stored_raw = static_cast<uint64_t>(r) & mask;
        } else {
            uint64_t a = g_programmer.stored_raw & mask,
                     b = g_programmer.current_raw & mask, r = 0;
            switch (g_programmer.pending_op) {
            case L'+':
                r = a + b;
                break;
            case L'-':
                r = a - b;
                break;
            case L'*':
                r = a * b;
                break;
            case L'/':
                r = b ? a / b : 0;
                break;
            default:
                return false;
            }
            g_programmer.stored_raw = r & mask;
        }
    } else {
        long double a = raw_to_long_double(g_programmer.stored_raw,
                                           g_programmer.type),
                    b = raw_to_long_double(g_programmer.current_raw,
                                           g_programmer.type),
                    r = 0;
        switch (g_programmer.pending_op) {
        case L'+':
            r = a + b;
            break;
        case L'-':
            r = a - b;
            break;
        case L'*':
            r = a * b;
            break;
        case L'/':
            r = b ? a / b : 0;
            break;
        default:
            return false;
        }
        g_programmer.stored_raw = cast_long_double_to_raw(r, g_programmer.type);
    }
    clear_programmer_exact_input();
    return true;
}

void programmer_input_operator(wchar_t op) {
    if (!apply_pending_programmer_operation())
        return;
    auto v = format_programmer_value(g_programmer.stored_raw, g_programmer.type,
                                     g_programmer.base);
    g_programmer.history = v + L" " + std::wstring(1, op);
    g_programmer.pending_op = op;
    g_programmer.current_raw = g_programmer.stored_raw;
    g_programmer.reset_input = true;
    g_programmer.just_evaluated = false;
    sync_programmer_display_from_state();
}

void programmer_evaluate() {
    if (!apply_pending_programmer_operation())
        return;
    auto cur = format_programmer_value(g_programmer.current_raw,
                                       g_programmer.type, g_programmer.base);
    g_programmer.history = g_programmer.pending_op
                               ? g_programmer.history + L" " + cur + L" ="
                               : cur + L" =";
    g_programmer.current_raw = g_programmer.stored_raw;
    g_programmer.pending_op = 0;
    g_programmer.reset_input = g_programmer.just_evaluated = true;
    sync_programmer_display_from_state();
}

void programmer_toggle_bit(int bit) {
    if (bit < 0 || bit >= bit_width(g_programmer.type))
        return;
    clear_programmer_exact_input();
    g_programmer.current_raw ^= 1ull << bit;
    g_programmer.current_raw &= mask_for_bits(bit_width(g_programmer.type));
    g_programmer.just_evaluated = false;
    sync_programmer_display_from_state();
}

void set_programmer_type(DataType t) {
    if (g_programmer.type == t)
        return;
    commit_exact_programmer_input(false);
    auto cur = raw_to_long_double(g_programmer.current_raw, g_programmer.type),
         st = raw_to_long_double(g_programmer.stored_raw, g_programmer.type);
    g_programmer.type = t;
    g_programmer.current_raw =
        cast_long_double_to_raw(cur, t) & mask_for_bits(bit_width(t));
    g_programmer.stored_raw =
        cast_long_double_to_raw(st, t) & mask_for_bits(bit_width(t));
    clear_programmer_exact_input();
    SendMessageW(g_programmer.hTypeCombo, CB_SETCURSEL, idx(t), 0);
    update_buttons_enabled();
    sync_programmer_display_from_state();
}

void set_programmer_base(NumberBase b) {
    if (g_programmer.base == b)
        return;
    commit_exact_programmer_input(false);
    g_programmer.base = b;
    clear_programmer_exact_input();
    SendMessageW(g_programmer.hBaseCombo, CB_SETCURSEL, idx(b), 0);
    update_buttons_enabled();
    update_menu_checks();
    sync_programmer_display_from_state();
}

void sync_basic_from_programmer() {
    g_current_input = format_programmer_value(
        g_programmer.current_raw, g_programmer.type, NumberBase::Dec);
    g_history.clear();
    g_pending_op = 0;
    g_stored_value = 0;
    g_reset_input = g_just_evaluated = false;
}

void sync_programmer_from_basic() {
    double v = 0;
    if (!try_parse_number(g_current_input, 10, v))
        v = 0;
    g_programmer.current_raw = cast_long_double_to_raw(v, g_programmer.type) &
                               mask_for_bits(bit_width(g_programmer.type));
    g_programmer.stored_raw = g_programmer.current_raw;
    g_programmer.pending_op = 0;
    g_programmer.history.clear();
    g_programmer.reset_input = g_programmer.just_evaluated = false;
    clear_programmer_exact_input();
}

void set_window_alpha(BYTE alpha) {
    LONG_PTR ex = GetWindowLongPtrW(g_window_handle, GWL_EXSTYLE);
    if (!(ex & WS_EX_LAYERED))
        SetWindowLongPtrW(g_window_handle, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    SetLayeredWindowAttributes(g_window_handle, 0, alpha, LWA_ALPHA);
}

void clear_window_alpha() {
    LONG_PTR ex = GetWindowLongPtrW(g_window_handle, GWL_EXSTYLE);
    if (!(ex & WS_EX_LAYERED))
        return;
    SetWindowLongPtrW(g_window_handle, GWL_EXSTYLE, ex & ~WS_EX_LAYERED);
    SetWindowPos(g_window_handle, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED |
                     SWP_NOACTIVATE);
}

void apply_topmost_state() {
    SetWindowPos(g_window_handle, g_is_topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    g_is_topmost ? set_window_alpha((GetActiveWindow() == g_window_handle ||
                                     GetForegroundWindow() == g_window_handle)
                                        ? 255
                                        : kInactiveAlpha)
                 : clear_window_alpha();
}

int read_ini_int(const wchar_t *sec, const wchar_t *key, int def) {
    return GetPrivateProfileIntW(sec, key, def, g_ini_path.c_str());
}
void write_ini_int(const wchar_t *sec, const wchar_t *key, int v) {
    wchar_t b[64]{};
    wsprintfW(b, L"%d", v);
    WritePrivateProfileStringW(sec, key, b, g_ini_path.c_str());
}

AppConfig load_config() {
    AppConfig c;
    c.x = read_ini_int(L"Window", L"X", CW_USEDEFAULT);
    c.y = read_ini_int(L"Window", L"Y", CW_USEDEFAULT);
    c.w = read_ini_int(L"Window", L"W", 900);
    c.h = read_ini_int(L"Window", L"H", 700);
    c.mode = static_cast<AppMode>(
        read_ini_int(L"Settings", L"Mode", idx(AppMode::Basic)));
    c.type = static_cast<DataType>(
        read_ini_int(L"Settings", L"DataType", idx(DataType::Int32)));
    c.base = static_cast<NumberBase>(
        read_ini_int(L"Settings", L"Base", idx(NumberBase::Dec)));
    c.separators = read_ini_int(L"Settings", L"Separators", 1) != 0;
    c.topmost = read_ini_int(L"Settings", L"TopMost", 0) != 0;
    return c;
}

void save_config() {
    RECT rc{};
    GetWindowRect(g_window_handle, &rc);
    write_ini_int(L"Window", L"X", rc.left);
    write_ini_int(L"Window", L"Y", rc.top);
    write_ini_int(L"Window", L"W", rc.right - rc.left);
    write_ini_int(L"Window", L"H", rc.bottom - rc.top);
    write_ini_int(L"Settings", L"Mode", idx(g_mode));
    write_ini_int(L"Settings", L"DataType", idx(g_programmer.type));
    write_ini_int(L"Settings", L"Base", idx(g_programmer.base));
    write_ini_int(L"Settings", L"Separators", g_use_separators);
    write_ini_int(L"Settings", L"TopMost", g_is_topmost);
}

void update_menu_checks() {
    HMENU m = GetMenu(g_window_handle);
    auto chk = [&](int id, bool on) {
        CheckMenuItem(m, id, MF_BYCOMMAND | (on ? MF_CHECKED : MF_UNCHECKED));
    };
    chk(ID_MODE_BASIC, g_mode == AppMode::Basic);
    chk(ID_MODE_PROGRAMMER, g_mode == AppMode::Programmer);
    chk(ID_VIEW_HEX, g_programmer.base == NumberBase::Hex);
    chk(ID_VIEW_DEC, g_programmer.base == NumberBase::Dec);
    chk(ID_VIEW_OCT, g_programmer.base == NumberBase::Oct);
    chk(ID_VIEW_BIN, g_programmer.base == NumberBase::Bin);
    chk(ID_VIEW_SEPARATORS, g_use_separators);
    chk(ID_VIEW_ALWAYSONTOP, g_is_topmost);
    UINT state = g_mode == AppMode::Programmer ? MF_ENABLED : MF_GRAYED;
    for (int id : {ID_VIEW_HEX, ID_VIEW_DEC, ID_VIEW_OCT, ID_VIEW_BIN})
        EnableMenuItem(m, id, MF_BYCOMMAND | state);
    DrawMenuBar(g_window_handle);
}

void set_mode(AppMode m) {
    if (g_mode == m)
        return;
    m == AppMode::Programmer ? sync_programmer_from_basic()
                             : sync_basic_from_programmer();
    g_mode = m;
    ShowWindow(g_programmer.hTypeCombo,
               g_mode == AppMode::Programmer ? SW_SHOW : SW_HIDE);
    ShowWindow(g_programmer.hBaseCombo, SW_HIDE);
    ShowWindow(g_programmer.hBitDisplay,
               g_mode == AppMode::Programmer ? SW_SHOW : SW_HIDE);
    update_buttons_enabled();
    update_menu_checks();
    SendMessageW(g_window_handle, WM_SIZE, 0, 0);
    sync_display_from_state();
}

HFONT make_font(int px, int weight) {
    return CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                       L"Segoe UI");
}
void reset_font(HFONT &f) {
    if (f)
        DeleteObject(f);
    f = nullptr;
}

void resize_display_fonts(HWND hwnd, int w, int h) {
    auto *ds = reinterpret_cast<DisplayState *>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!ds)
        return;
    reset_font(ds->hist);
    reset_font(ds->main);
    reset_font(ds->warn);
    ds->hist = make_font(std::max(12, h / 8), FW_NORMAL);
    ds->main = make_font(std::max(18, std::min(h / 3, w / 10)), FW_BOLD);
    ds->warn = make_font(std::max(11, h / 9), FW_NORMAL);
    InvalidateRect(hwnd, nullptr, TRUE);
}

struct BitBoxLayout {
    RECT rc{};
    int rows = 1, cols = 1, boxW = 1, boxH = 1, bitCount = 1;
};
BitBoxLayout get_bit_layout(const RECT &rc) {
    BitBoxLayout l;
    l.rc = rc;
    l.bitCount = bit_width(g_programmer.type);
    l.cols = std::min(16, l.bitCount);
    l.rows = std::max(1, (l.bitCount + l.cols - 1) / l.cols);
    constexpr int pad = 8, gx = 4, gy = 8;
    int uw = std::max(1, int(rc.right - rc.left) - pad * 2 - (l.cols - 1) * gx),
        uh = std::max(1, int(rc.bottom - rc.top) - pad * 2 - (l.rows - 1) * gy);
    l.boxW = std::max(10, uw / l.cols);
    l.boxH = std::max(l.rows >= 4 ? 18 : 28, uh / l.rows);
    return l;
}

std::wstring tooltip_for_bit(int bit) {
    int bits = bit_width(g_programmer.type);
    if (bit < 0 || bit >= bits)
        return L"";
    if (is_float_type(g_programmer.type)) {
        const TypeMeta &type_info = meta(g_programmer.type);
        int eb = type_info.exp, mb = type_info.mant, sign = bits - 1;
        if (bit == sign)
            return L"Sign bit";
        if (bit >= mb && bit <= mb + eb - 1) {
            std::wostringstream os;
            os << L"Exponent bit: 2^" << (bit - mb) << L" (Bias: "
               << type_info.bias << L")";
            return os.str();
        }
        std::wostringstream os;
        os << L"Mantissa bit: 2^" << (bit - mb);
        return os.str();
    }
    std::wostringstream os;
    if (is_signed_integer_type(g_programmer.type) && bit == bits - 1)
        os << L"Sign bit: -2^" << bits - 1;
    else
        os << L"Bit " << bit << L": 2^" << bit;
    return os.str();
}

int hit_test_bit(HWND hwnd, int x, int y) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    auto l = get_bit_layout(rc);
    constexpr int px = 8, py = 8, gx = 4, gy = 8;
    for (int v = 0; v < l.bitCount; ++v) {
        int r = v / l.cols, c = v % l.cols;
        RECT box{px + c * (l.boxW + gx), py + r * (l.boxH + gy),
                 px + c * (l.boxW + gx) + l.boxW,
                 py + r * (l.boxH + gy) + l.boxH};
        if (x >= box.left && x < box.right && y >= box.top && y < box.bottom)
            return l.bitCount - 1 - v;
    }
    return -1;
}

void update_bit_tooltip(HWND hwnd, int bit, POINT ptClient) {
    HWND tt = reinterpret_cast<HWND>(GetPropW(hwnd, L"BIT_TOOLTIP"));
    if (!tt)
        return;
    TOOLINFOW ti{sizeof(ti)};
    ti.uFlags = TTF_TRACK | TTF_ABSOLUTE | TTF_IDISHWND;
    ti.hwnd = hwnd;
    ti.uId = reinterpret_cast<UINT_PTR>(hwnd);
    if (bit < 0)
        return (void)SendMessageW(tt, TTM_TRACKACTIVATE, FALSE,
                                  reinterpret_cast<LPARAM>(&ti));
    g_programmer.tooltip_text = tooltip_for_bit(bit);
    ti.lpszText = const_cast<wchar_t *>(g_programmer.tooltip_text.c_str());
    SendMessageW(tt, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&ti));
    ClientToScreen(hwnd, &ptClient);
    SendMessageW(tt, TTM_TRACKPOSITION, 0,
                 MAKELPARAM(ptClient.x + 14, ptClient.y + 24));
    SendMessageW(tt, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
}

void hide_all_buttons() {
    for (auto &b : g_buttons)
        ShowWindow(b.hwnd, SW_HIDE);
}

void layout_basic_grid_buttons(int top, int rowH, int colW, int contentW,
                               int pad) {
    for (int i = 0; i < (int)kBasicLayout.size(); ++i) {
        int row = i / 4, col = i % 4, y = top + row * (rowH + pad);
        HWND h = find_button(kBasicLayout[i]);
        if (!h)
            continue;
        if (i == 16)
            MoveWindow(h, pad, top + 4 * (rowH + pad), colW, rowH, TRUE);
        else if (i == 17)
            MoveWindow(h, pad + colW + pad, top + 4 * (rowH + pad),
                       contentW - colW - pad, rowH, TRUE);
        else
            MoveWindow(h, pad + col * (colW + pad), y, colW, rowH, TRUE);
        ShowWindow(h, SW_SHOW);
    }
}

void layout_children(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    constexpr int pad = 5;
    int w = rc.right - rc.left, h = rc.bottom - rc.top, contentW = w - 2 * pad;
    if (g_mode == AppMode::Basic) {
        int dispH = std::max(70, h / 4 - pad);
        MoveWindow(g_display_handle, pad, pad, contentW, dispH, TRUE);
        int top = pad + dispH + pad, areaH = h - top - pad,
            rowH = std::max(28, (areaH - 4 * pad) / 5),
            colW = std::max(40, (contentW - 3 * pad) / 4);
        hide_all_buttons();
        layout_basic_grid_buttons(top, rowH, colW, contentW, pad);
        ShowWindow(g_programmer.hTypeCombo, SW_HIDE);
        ShowWindow(g_programmer.hBaseCombo, SW_HIDE);
        ShowWindow(g_programmer.hBitDisplay, SW_HIDE);
        update_buttons_enabled();
        return;
    }
    int dispH = std::max(72, h / 5), comboTop = pad + dispH + pad,
        bitH = std::max(72, h / 5);
    MoveWindow(g_display_handle, pad, pad, contentW, dispH, TRUE);
    MoveWindow(g_programmer.hTypeCombo, pad, comboTop, contentW, 260, TRUE);
    ShowWindow(g_programmer.hBaseCombo, SW_HIDE);
    MoveWindow(g_programmer.hBitDisplay, pad, comboTop + 28 + pad, contentW,
               bitH, TRUE);
    int buttonsTop = comboTop + 28 + pad + bitH + pad,
        buttonsH = h - buttonsTop - pad,
        rowH = std::max(28, (buttonsH - 5 * pad) / 6),
        letterW = std::max(28, (contentW - 5 * pad) / 6),
        colW = std::max(40, (contentW - 3 * pad) / 4);
    hide_all_buttons();
    for (int i = 0; i < 6; ++i)
        if (HWND hBtn = find_button(kLetterButtons[i]))
            MoveWindow(hBtn, pad + i * (letterW + pad), buttonsTop, letterW,
                       rowH, TRUE),
                ShowWindow(hBtn, SW_SHOW);
    layout_basic_grid_buttons(buttonsTop + rowH + pad, rowH, colW, contentW,
                              pad);
    update_buttons_enabled();
}

void update_buttons_enabled() {
    auto en = [&](int id, bool on) {
        if (HWND h = find_button(id))
            EnableWindow(h, on);
    };
    if (g_mode == AppMode::Basic) {
        for (auto &b : g_buttons)
            EnableWindow(b.hwnd, TRUE);
        for (int id : kLetterButtons)
            en(id, false);
        return;
    }
    bool hex = g_programmer.base == NumberBase::Hex,
         dec = g_programmer.base == NumberBase::Dec,
         oct = g_programmer.base == NumberBase::Oct;
    for (int id : kLetterButtons)
        en(id, hex);
    for (auto [id, v] : kDigitButtons)
        en(id,
           hex || dec || (oct && v <= 7) || (!oct && !dec && !hex && v <= 1));
    en(ID_BTN_DOT, dot_allowed_for_programmer());
}

void handle_button_command(int id) {
    auto digit = digit_from_button_id(id), hex = hex_from_button_id(id),
         op = op_from_button_id(id);
    if (g_mode == AppMode::Basic) {
        if (digit)
            return basic_input_digit(digit);
        if (op)
            return basic_input_operator(op);
        if (id == ID_BTN_DOT)
            return basic_input_dot();
        if (id == ID_BTN_BS)
            return basic_backspace();
        if (id == ID_BTN_CLEAR)
            return clear_all();
        if (id == ID_BTN_EQ)
            return basic_evaluate();
        return;
    }
    if (hex)
        return programmer_input_char(hex);
    if (digit)
        return programmer_input_char(digit);
    if (op)
        return programmer_input_operator(op);
    if (id == ID_BTN_DOT)
        return programmer_input_dot();
    if (id == ID_BTN_BS)
        return programmer_backspace();
    if (id == ID_BTN_CLEAR)
        return clear_all();
    if (id == ID_BTN_EQ)
        return programmer_evaluate();
}

LRESULT CALLBACK display_window_proc(HWND hwnd, UINT msg, WPARAM wp,
                                     LPARAM lp) {
    switch (msg) {
    case WM_NCCREATE:
        SetWindowLongPtrW(
            hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(
                reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams));
        return TRUE;
    case WM_SIZE:
        resize_display_fonts(hwnd, LOWORD(lp), HIWORD(lp));
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h),
                oldBmp = static_cast<HBITMAP>(SelectObject(mem, bmp));
        HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(mem, &rc, bg);
        DeleteObject(bg);
        SetBkMode(mem, TRANSPARENT);
        auto *ds = reinterpret_cast<DisplayState *>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (ds) {
            RECT hist = rc, main = rc, warn = rc;
            int pad = 12, warnH = ds->warning.empty() ? 0 : std::max(20, h / 5);
            hist.left += pad;
            hist.right -= pad;
            hist.top += pad;
            hist.bottom = rc.top + h / 3;
            warn.left += pad;
            warn.right -= pad;
            warn.bottom -= pad;
            warn.top = warn.bottom - warnH;
            main.left += pad;
            main.right -= pad;
            main.top = hist.bottom - 4;
            main.bottom = ds->warning.empty() ? rc.bottom - pad : warn.top - 2;
            HFONT old = nullptr;
            if (ds->hist)
                old = static_cast<HFONT>(SelectObject(mem, ds->hist));
            SetTextColor(mem, RGB(96, 96, 96));
            DrawTextW(mem, ds->history.c_str(), -1, &hist,
                      DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            if (ds->main)
                SelectObject(mem, ds->main);
            SetTextColor(mem, RGB(0, 0, 0));
            DrawTextW(mem, ds->current.c_str(), -1, &main,
                      DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            if (!ds->warning.empty()) {
                if (ds->warn)
                    SelectObject(mem, ds->warn);
                SetTextColor(mem, RGB(180, 0, 0));
                DrawTextW(mem, ds->warning.c_str(), -1, &warn,
                          DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            }
            if (old)
                SelectObject(mem, old);
        }
        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: {
        auto *ds = reinterpret_cast<DisplayState *>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (!ds)
            return 0;
        reset_font(ds->hist);
        reset_font(ds->main);
        reset_font(ds->warn);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK bit_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        HWND tt =
            CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, hwnd, nullptr, g_instance, nullptr);
        TOOLINFOW ti{sizeof(ti)};
        ti.uFlags = TTF_TRACK | TTF_ABSOLUTE | TTF_IDISHWND;
        ti.hwnd = hwnd;
        ti.uId = reinterpret_cast<UINT_PTR>(hwnd);
        ti.lpszText = const_cast<wchar_t *>(L"");
        SendMessageW(tt, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
        SetPropW(hwnd, L"BIT_TOOLTIP", tt);
        return 0;
    }
    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&t);
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        int bit = hit_test_bit(hwnd, pt.x, pt.y);
        if (bit != g_programmer.hovered_bit) {
            g_programmer.hovered_bit = bit;
            update_bit_tooltip(hwnd, bit, pt);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        g_programmer.hovered_bit = -1;
        update_bit_tooltip(hwnd, -1, {});
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_LBUTTONDOWN:
        programmer_toggle_bit(
            hit_test_bit(hwnd, GET_X_LPARAM(lp), GET_Y_LPARAM(lp)));
        SetFocus(g_window_handle);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        HDC mem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h),
                oldBmp = static_cast<HBITMAP>(SelectObject(mem, bmp));
        HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(mem, &rc, bg);
        DeleteObject(bg);
        auto l = get_bit_layout(rc);
        constexpr int px = 8, py = 8, gx = 4, gy = 8;
        int bitPx = std::max(12, std::min(l.boxH / 2, l.boxW - 2)),
            idxPx = std::max(8, bitPx / 3);
        HFONT bitF = make_font(bitPx, FW_BOLD),
              idxF = make_font(idxPx, FW_NORMAL);
        SetBkMode(mem, TRANSPARENT);
        for (int v = 0; v < l.bitCount; ++v) {
            int row = v / l.cols, col = v % l.cols, bit = l.bitCount - 1 - v;
            RECT cell{px + col * (l.boxW + gx), py + row * (l.boxH + gy),
                      px + col * (l.boxW + gx) + l.boxW,
                      py + row * (l.boxH + gy) + l.boxH};
            bool set = ((g_programmer.current_raw >> bit) & 1ull) != 0,
                 hover = bit == g_programmer.hovered_bit;
            int digitH = (l.boxH * 2) / 3;
            RECT br{cell.left, cell.top, cell.right, cell.top + digitH},
                ir{cell.left, cell.top + digitH, cell.right, cell.bottom};
            SelectObject(mem, bitF);
            SetTextColor(mem, hover ? RGB(20, 20, 20)
                              : set ? RGB(25, 25, 25)
                                    : RGB(170, 170, 170));
            DrawTextW(mem, set ? L"1" : L"0", -1, &br,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            auto idxText = std::to_wstring(bit);
            SelectObject(mem, idxF);
            SetTextColor(mem, hover ? RGB(70, 70, 70) : RGB(165, 165, 165));
            DrawTextW(mem, idxText.c_str(), -1, &ir,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (hover) {
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(110, 110, 110)),
                     oldPen = static_cast<HPEN>(SelectObject(mem, pen));
                MoveToEx(mem, cell.left + 2, cell.top + digitH - 1, nullptr);
                LineTo(mem, cell.right - 2, cell.top + digitH - 1);
                SelectObject(mem, oldPen);
                DeleteObject(pen);
            }
        }
        DeleteObject(bitF);
        DeleteObject(idxF);
        BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        if (HWND tt = reinterpret_cast<HWND>(GetPropW(hwnd, L"BIT_TOOLTIP")))
            DestroyWindow(tt), RemovePropW(hwnd, L"BIT_TOOLTIP");
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK calc_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_window_handle = hwnd;
        g_display_handle =
            CreateWindowExW(0, L"Display", nullptr, WS_CHILD | WS_VISIBLE, 0, 0,
                            100, 100, hwnd, nullptr, g_instance, &g_display);
        g_programmer.hTypeCombo = CreateWindowExW(
            0, WC_COMBOBOXW, nullptr,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            0, 0, 100, 300, hwnd, reinterpret_cast<HMENU>(ID_COMBO_TYPE),
            g_instance, nullptr);
        g_programmer.hBaseCombo = CreateWindowExW(
            0, WC_COMBOBOXW, nullptr,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
            0, 0, 100, 200, hwnd, reinterpret_cast<HMENU>(ID_COMBO_BASE),
            g_instance, nullptr);
        g_programmer.hBitDisplay =
            CreateWindowExW(0, L"BitDisplay", nullptr, WS_CHILD | WS_VISIBLE, 0,
                            0, 100, 100, hwnd, nullptr, g_instance, nullptr);
        for (int i = 0; i <= idx(DataType::Float64); ++i)
            SendMessageW(
                g_programmer.hTypeCombo, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>(type_name(static_cast<DataType>(i))));
        for (int i = 0; i <= idx(NumberBase::Bin); ++i)
            SendMessageW(g_programmer.hBaseCombo, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(
                             base_name(static_cast<NumberBase>(i))));
        for (auto def : kButtons) {
            def.hwnd = CreateWindowExW(
                0, L"BUTTON", def.text,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(def.id)),
                g_instance, nullptr);
            g_buttons.push_back(def);
        }
        g_is_topmost = g_initial_config.topmost;
        g_use_separators = g_initial_config.separators;
        g_mode = g_initial_config.mode;
        g_programmer.type = g_initial_config.type;
        g_programmer.base = g_initial_config.base;
        SendMessageW(g_programmer.hTypeCombo, CB_SETCURSEL,
                     idx(g_programmer.type), 0);
        SendMessageW(g_programmer.hBaseCombo, CB_SETCURSEL,
                     idx(g_programmer.base), 0);
        if (g_mode == AppMode::Programmer)
            sync_programmer_from_basic();
        ShowWindow(g_programmer.hTypeCombo,
                   g_mode == AppMode::Programmer ? SW_SHOW : SW_HIDE);
        ShowWindow(g_programmer.hBaseCombo, SW_HIDE);
        ShowWindow(g_programmer.hBitDisplay,
                   g_mode == AppMode::Programmer ? SW_SHOW : SW_HIDE);
        apply_topmost_state();
        update_menu_checks();
        sync_display_from_state();
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto *m = reinterpret_cast<MINMAXINFO *>(lp);
        m->ptMinTrackSize.x = 320;
        m->ptMinTrackSize.y = 400;
        return 0;
    }
    case WM_SIZE:
        layout_children(hwnd);
        return 0;
    case WM_ACTIVATE:
        if (g_is_topmost)
            set_window_alpha(LOWORD(wp) != WA_INACTIVE ? 255 : kInactiveAlpha);
        return 0;
    case WM_CHAR: {
        wchar_t ch = static_cast<wchar_t>(wp);
        if (ch == L'\b') {
            if (g_mode == AppMode::Basic)
                basic_backspace();
            else
                programmer_backspace();
            return 0;
        }
        if (ch == L'\r') {
            if (g_mode == AppMode::Basic)
                basic_evaluate();
            else
                programmer_evaluate();
            return 0;
        }
        if (ch == L'.') {
            if (g_mode == AppMode::Basic)
                basic_input_dot();
            else
                programmer_input_dot();
            return 0;
        }
        if (ch == L'+' || ch == L'-' || ch == L'*' || ch == L'/') {
            if (g_mode == AppMode::Basic)
                basic_input_operator(ch);
            else
                programmer_input_operator(ch);
            return 0;
        }
        if (ch >= L'1' && ch <= L'9') {
            if (g_mode == AppMode::Basic) {
                basic_input_digit(ch);
                return 0;
            }
            bool allow_digit =
                g_programmer.base == NumberBase::Hex ||
                g_programmer.base == NumberBase::Dec ||
                (g_programmer.base == NumberBase::Oct && ch <= L'7') ||
                (g_programmer.base == NumberBase::Bin && ch == L'1');
            if (allow_digit) {
                programmer_input_char(ch);
                return 0;
            }
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == ID_COMBO_TYPE) {
            if (code == CBN_DROPDOWN)
                return 0;
            if (int sel = (int)SendMessageW(g_programmer.hTypeCombo,
                                            CB_GETCURSEL, 0, 0);
                sel >= 0)
                set_programmer_type(static_cast<DataType>(sel));
            update_buttons_enabled();
            return 0;
        }
        if (id == ID_COMBO_BASE && code == CBN_SELCHANGE) {
            if (int sel = (int)SendMessageW(g_programmer.hBaseCombo,
                                            CB_GETCURSEL, 0, 0);
                sel >= 0)
                set_programmer_base(static_cast<NumberBase>(sel));
            update_menu_checks();
            update_buttons_enabled();
            return 0;
        }
        switch (id) {
        case ID_MODE_BASIC:
            set_mode(AppMode::Basic);
            return 0;
        case ID_MODE_PROGRAMMER:
            set_mode(AppMode::Programmer);
            return 0;
        case ID_EDIT_CLEAR:
            clear_all();
            return 0;
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            return 0;
        case ID_HELP_ABOUT:
            MessageBoxW(hwnd, L"DevCalculator", L"About DevCalculator",
                        MB_OK | MB_ICONINFORMATION);
            return 0;
        case ID_VIEW_ALWAYSONTOP:
            g_is_topmost = !g_is_topmost;
            apply_topmost_state();
            update_menu_checks();
            return 0;
        case ID_VIEW_HEX:
            set_programmer_base(NumberBase::Hex);
            update_menu_checks();
            update_buttons_enabled();
            return 0;
        case ID_VIEW_DEC:
            set_programmer_base(NumberBase::Dec);
            update_menu_checks();
            update_buttons_enabled();
            return 0;
        case ID_VIEW_OCT:
            set_programmer_base(NumberBase::Oct);
            update_menu_checks();
            update_buttons_enabled();
            return 0;
        case ID_VIEW_BIN:
            set_programmer_base(NumberBase::Bin);
            update_menu_checks();
            update_buttons_enabled();
            return 0;
        case ID_VIEW_SEPARATORS:
            g_use_separators = !g_use_separators;
            sync_display_from_state();
            update_menu_checks();
            return 0;
        }
        handle_button_command(id);
        return 0;
    }
    case WM_DESTROY:
        save_config();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void register_window_class(HINSTANCE inst, const wchar_t *name, WNDPROC proc,
                           HCURSOR cur, HICON icon = nullptr) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.hIcon = icon;
    wc.hCursor = cur;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = name;
    RegisterClassExW(&wc);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    g_instance = instance;
    wchar_t module_path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, module_path, MAX_PATH);
    g_ini_path = module_path;
    if (auto slash_pos = g_ini_path.find_last_of(L"\\/");
        slash_pos != std::wstring::npos)
        g_ini_path.erase(slash_pos + 1);
    g_ini_path += L"config.ini";
    g_initial_config = load_config();
    register_window_class(instance, L"Main", calc_window_proc,
                          LoadCursorW(nullptr, IDC_ARROW),
                          LoadIconW(instance, MAKEINTRESOURCEW(ID_ICON)));
    register_window_class(instance, L"Display", display_window_proc,
                          LoadCursorW(nullptr, IDC_ARROW));
    register_window_class(instance, L"BitDisplay", bit_window_proc,
                          LoadCursorW(nullptr, IDC_HAND));
    HWND hwnd = CreateWindowExW(
        0, L"Main", L"DevCalculator", WS_OVERLAPPEDWINDOW, g_initial_config.x,
        g_initial_config.y, g_initial_config.w, g_initial_config.h, nullptr,
        LoadMenuW(instance, MAKEINTRESOURCEW(ID_MAIN_MENU)), instance, nullptr);
    g_window_handle = hwnd;
    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);
    HACCEL acc = LoadAcceleratorsW(instance, MAKEINTRESOURCEW(ID_ACCELERATORS));
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
        if (!TranslateAcceleratorW(hwnd, acc, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    return static_cast<int>(msg.wParam);
}
