#include "socket_reader.hpp"
#include <iostream>

namespace crocofix
{

socket_reader::socket_reader(boost::asio::ip::tcp::socket& socket)
:   m_socket(socket)
{
}

void socket_reader::read_async(reader::message_callback callback)
{
    if (m_write_offset == m_read_buffer.size()) 
    {
        // The buffer is full.
        auto remainder_size = m_write_offset - m_read_offset;
    
        if (remainder_size > 0) {
            // We have some left over data, copy the left overs back to the start of the buffer
            // so we have room to read more bytes. Due to the way message::decode works this can
            // only be less than the length of one tag/value pair.
            memcpy(&m_read_buffer[0], &m_read_buffer[m_read_offset], remainder_size);
        }
    
        m_read_offset = 0;
        m_write_offset = remainder_size;
    }

    auto buffer = boost::asio::buffer(&*m_read_buffer.begin() + m_write_offset, 
                                      m_read_buffer.size() - m_write_offset);

    m_socket.async_read_some(buffer, [&, callback](const boost::system::error_code& error, std::size_t bytes_transferred) 
    {
        if (error) {
            throw boost::system::system_error(error);
        }

        m_write_offset += bytes_transferred;   

        while (true) 
        {
            auto [consumed, complete] = m_message.decode(std::string_view(&m_read_buffer[m_read_offset], m_write_offset - m_read_offset));

            m_read_offset += consumed;

            if (!complete) {
                break;
            }

            callback(m_message);
        
            m_message.fields().clear();
        }

        read_async(callback);
    });
}

}