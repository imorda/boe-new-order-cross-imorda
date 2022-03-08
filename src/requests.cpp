#include "requests.h"

namespace {

void encode_new_order_opt_fields(unsigned char * bitfield_start,
                                 const double price,
                                 const char ord_type,
                                 const char time_in_force,
                                 const unsigned max_floor,
                                 const std::string & symbol,
                                 const char capacity,
                                 const std::string & account)
{
    auto * p = bitfield_start + new_order_bitfield_num();
#define FIELD(name, bitfield_num, bit)                    \
    set_opt_field_bit(bitfield_start, bitfield_num, bit); \
    p = encode_field_##name(p, name);
#include "new_order_opt_fields.inl"
}

unsigned char * declare_new_order_cross_opt_fields(unsigned char * bitfield_start)
{
#define FIELD(name, bitfield_num, bit) \
    set_opt_field_bit(bitfield_start, bitfield_num, bit);
#include "new_order_cross_single_opt_fields.inl"
#define FIELD(name, bitfield_num, bit) \
    set_opt_field_bit(bitfield_start, bitfield_num, bit);
#include "new_order_cross_repeated_opt_fields.inl"

    return bitfield_start + new_order_cross_bitfield_num();
}

unsigned char * encode_new_order_cross_repeated_opt_fields(unsigned char * p,
                                                           const char algo_ind)
{
#define FIELD(name, bitfield_num, bit) \
    p = encode_field_##name(p, name);
#include "new_order_cross_repeated_opt_fields.inl"
    return p;
}

unsigned char * encode_new_order_cross_single_opt_fields(unsigned char * p,
                                                         const std::string & symbol)
{
#define FIELD(name, bitfield_num, bit) \
    p = encode_field_##name(p, name);
#include "new_order_cross_single_opt_fields.inl"
    return p;
}

unsigned char * declare_new_order_cross_multileg_opt_fields(unsigned char * bitfield_start)
{
#define FIELD(name, bitfield_num, bit) \
    set_opt_field_bit(bitfield_start, bitfield_num, bit);
#include "new_order_cross_multileg_single_opt_fields.inl"
#define FIELD(name, bitfield_num, bit) \
    set_opt_field_bit(bitfield_start, bitfield_num, bit);
#include "new_order_cross_multileg_repeated_opt_fields.inl"

    return bitfield_start + new_order_cross_multileg_bitfield_num();
}

unsigned char * encode_new_order_cross_multileg_repeated_opt_fields(unsigned char * p,
                                                                    const char algo_ind,
                                                                    const std::string & leg_pos_effect)
{
#define FIELD(name, bitfield_num, bit) \
    p = encode_field_##name(p, name);
#include "new_order_cross_multileg_repeated_opt_fields.inl"
    return p;
}

unsigned char * encode_new_order_cross_multileg_single_opt_fields(unsigned char * p,
                                                                  const std::string & symbol)
{
#define FIELD(name, bitfield_num, bit) \
    p = encode_field_##name(p, name);
#include "new_order_cross_multileg_single_opt_fields.inl"
    return p;
}

uint8_t encode_request_type(const RequestType type)
{
    switch (type) {
    case RequestType::New:
        return 0x38;
    case RequestType::NewCross:
        return 0x7A;
    case RequestType::NewCrossMultileg:
        return 0x85;
    }
}

unsigned char * add_request_header(unsigned char * start, unsigned length, const RequestType type, unsigned seq_no)
{
    *start++ = 0xBA;
    *start++ = 0xBA;
    start = encode(start, static_cast<uint16_t>(length));
    start = encode(start, encode_request_type(type));
    *start++ = 0;
    return encode(start, static_cast<uint32_t>(seq_no));
}

char convert_side(const Side side)
{
    switch (side) {
    case Side::Buy: return '1';
    case Side::Sell: return '2';
    }
    return 0;
}

char convert_ord_type(const OrdType ord_type)
{
    switch (ord_type) {
    case OrdType::Market: return '1';
    case OrdType::Limit: return '2';
    case OrdType::Pegged: return 'P';
    }
    return 0;
}

char convert_algo_ind(bool indicator)
{
    return indicator ? 'Y' : 'N';
}

char convert_time_in_force(const TimeInForce time_in_force)
{
    switch (time_in_force) {
    case TimeInForce::Day: return '0';
    case TimeInForce::IOC: return '3';
    case TimeInForce::GTD: return '6';
    }
    return 0;
}

char convert_capacity(const Capacity capacity)
{
    switch (capacity) {
    case Capacity::Agency: return 'A';
    case Capacity::Principal: return 'P';
    case Capacity::RisklessPrincipal: return 'R';
    }
    return 0;
}

char convert_account_type(const AccountType account_type)
{
    switch (account_type) {
    case AccountType::Client: return '1';
    case AccountType::House: return '3';
    }
    return 0;
}

char convert_position(const Position position)
{
    switch (position) {
    case Position::Open: return 'O';
    case Position::Close: return 'C';
    case Position::None: return 'N';
    }
    return 0;
}

std::string convert_legs(const std::vector<Position> & legs)
{
    std::string str;
    str.reserve(legs.size());
    for (const auto p : legs) {
        str += convert_position(p);
    }
    return str;
}

} // anonymous namespace

std::array<unsigned char, calculate_size(RequestType::New)> create_new_order_request(const unsigned seq_no,
                                                                                     const std::string & cl_ord_id,
                                                                                     const Side side,
                                                                                     const double volume,
                                                                                     const double price,
                                                                                     const OrdType ord_type,
                                                                                     const TimeInForce time_in_force,
                                                                                     const double max_floor,
                                                                                     const std::string & symbol,
                                                                                     const Capacity capacity,
                                                                                     const std::string & account)
{
    static_assert(calculate_size(RequestType::New) == 78, "Wrong New Order message size");

    std::array<unsigned char, calculate_size(RequestType::New)> msg;
    auto * p = add_request_header(&msg[0], msg.size() - 2, RequestType::New, seq_no);
    p = encode_text(p, cl_ord_id, 20);
    p = encode_char(p, convert_side(side));
    p = encode_binary4(p, static_cast<uint32_t>(volume));
    p = encode(p, static_cast<uint8_t>(new_order_bitfield_num()));
    encode_new_order_opt_fields(p,
                                price,
                                convert_ord_type(ord_type),
                                convert_time_in_force(time_in_force),
                                max_floor,
                                symbol,
                                convert_capacity(capacity),
                                account);
    return msg;
}

unsigned char * encode_cross_orders(unsigned char * p, const Order & order)
{
    p = encode_char(p, convert_side(order.side));
    p = encode_binary4(p, static_cast<uint32_t>(order.volume));
    p = encode_text(p, order.cl_ord_id, 20);
    p = encode_char(p, convert_capacity(order.capacity));
    p = encode_text(p, order.clearing_firm, 4);
    p = encode_char(p, convert_account_type(order.account_type));

    p = encode_new_order_cross_repeated_opt_fields(p,
                                                   convert_algo_ind(order.algorithmic_indicator));
    return p;
}
unsigned char * encode_new_order_cross_commons(unsigned char * p,
                                               const std::string & cross_id,
                                               double price,
                                               const Order & agency_order,
                                               const uint8_t bitfield_len)
{
    p = encode_text(p, cross_id, 20);
    p = encode_char(p, '1');
    p = encode_char(p, convert_side(agency_order.side));
    p = encode_price(p, price);
    p = encode_binary4(p, static_cast<uint32_t>(agency_order.volume));
    p = encode(p, bitfield_len);
    return p;
}
std::vector<unsigned char> create_new_order_cross_request(
        unsigned seq_no,
        const std::string & cross_id,
        double price,
        const std::string & symbol,
        const Order & agency_order,
        const std::vector<Order> & contra_orders)
{
    std::vector<unsigned char> msg;
    msg.resize(calculate_size(RequestType::NewCross, contra_orders.size()));

    auto * p = add_request_header(&msg[0], msg.size() - 2, RequestType::NewCross, seq_no);
    p = encode_new_order_cross_commons(p,
                                       cross_id,
                                       price,
                                       agency_order,
                                       new_order_cross_bitfield_num());
    p = declare_new_order_cross_opt_fields(p);
    p = encode_binary2(p, static_cast<uint16_t>(1 + contra_orders.size()));

    p = encode_cross_orders(p, agency_order);
    for (const auto & i : contra_orders) {
        p = encode_cross_orders(p, i);
    }

    encode_new_order_cross_single_opt_fields(p,
                                             symbol);

    return msg;
}

unsigned char * encode_cross_multileg_orders(unsigned char * p, const ComplexOrder & order)
{
    p = encode_char(p, convert_side(order.order.side));
    p = encode_binary4(p, static_cast<uint32_t>(order.order.volume));
    p = encode_text(p, order.order.cl_ord_id, 20);
    p = encode_char(p, convert_capacity(order.order.capacity));
    p = encode_text(p, order.order.clearing_firm, 4);
    p = encode_char(p, convert_account_type(order.order.account_type));

    p = encode_new_order_cross_multileg_repeated_opt_fields(p,
                                                            convert_algo_ind(order.order.algorithmic_indicator),
                                                            convert_legs(order.legs));
    return p;
}
std::vector<unsigned char> create_new_order_cross_multileg_request(
        unsigned seq_no,
        const std::string & cross_id,
        double price,
        const std::string & symbol,
        const ComplexOrder & agency_order,
        const std::vector<ComplexOrder> & contra_orders)
{
    std::vector<unsigned char> msg;
    msg.resize(calculate_size(RequestType::NewCrossMultileg, contra_orders.size()));

    auto * p = add_request_header(&msg[0], msg.size() - 2, RequestType::NewCrossMultileg, seq_no);

    p = encode_new_order_cross_commons(p,
                                       cross_id,
                                       price,
                                       agency_order.order,
                                       new_order_cross_multileg_bitfield_num());
    p = declare_new_order_cross_multileg_opt_fields(p);
    p = encode_binary2(p, static_cast<uint16_t>(1 + contra_orders.size()));

    p = encode_cross_multileg_orders(p, agency_order);
    for (const auto & i : contra_orders) {
        p = encode_cross_multileg_orders(p, i);
    }
    encode_new_order_cross_multileg_single_opt_fields(p,
                                                      symbol);

    return msg;
}
