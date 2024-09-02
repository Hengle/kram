#include "KramZipStream.h"

#include "miniz.h"

namespace kram {
using namespace NAMESPACE_STL;

ZipStream::ZipStream() {
    _stream = make_unique<mz_stream>();
}

// must be buried due to unique_ptr
ZipStream::~ZipStream() {
    close();
}

bool ZipStream::open() {
    KVERIFY(_fileHelper.isOpen());
    
    // https://www.zlib.net/zlib_how.html
    // https://www.ietf.org/rfc/rfc1952.txt
    
    // can also install custom allocators (allocates 256KB buffer otherwise)
    _stream->zalloc = NULL;
    _stream->zfree = NULL;
    _stream->opaque = NULL;
    
    // Just making this double the default mz_stream buffer.
    // Should be able to get about 2x compression (is there an estimator?).
    // TODO: what if input is bigger than output buffer?
    // The larger this number, the bigger the stall to compress.
    _compressLimit = 2*256*1024;
    
    KVERIFY(mz_deflateInit(_stream.get(), MZ_DEFAULT_LEVEL) == MZ_OK);
    
    // These are all optional fields
    enum GzipFlag : uint8_t {
        kGzipFlagText    = 1 << 0, // text 1, or ascii/uknown 0
        kGzipFlagCRC     = 1 << 1, // crc16 for header
        kGzipFlagExtra   = 1 << 2,
        kGzipFlagName    = 1 << 3, // null terminated filename
        kGzipFlagComment = 1 << 4, // null terminated comment
    };
    
    enum GzipPlatform : uint8_t {
        kGzipPlatformFAT  = 0,
        kGzipPlatformUnix = 3,
        kGzipPlatformMac  = 7,
        kGzipPlatformNT   = 11,
        kGzipPlatformDefault = 255,
    };
    
    // for deflate, but seem of little utility
    enum GzipCompression : uint8_t {
        kGzipCompressionUnknown  = 0,
        kGzipCompressionSmallest = 2,
        kGzipCompressionFastest  = 4,
    };
    
    // gzip 10B header
    const uint8_t header[10] = {
        0x1f, 0x8b,
        0x08, // (compression method - deflate)
        0x00, // flags
              // The time is in Unix format, i.e., seconds since 00:00:00 GMT, Jan.  1, 1970.
        0x00, 0x00, 0x00, 0x00,  // TODO: timestamp mtime - start of compression or of src file
        kGzipCompressionUnknown, // compression id
        kGzipPlatformMac         // os platform id
    };
    
    // Not writing any of the flagged fields.
    
    // clear the data
    _sourceCRC32 = 0;
    _sourceSize = 0;
    
    bool success = _fileHelper.write((const uint8_t*)&header, sizeof(header));
    if (!success) {
        KLOGE("ZipStream", "Could not write gzip header to %s", _fileHelper.filename().c_str());
    }
    
    return success;
    
    // zlib is slightly different than gzip format (11B overhead)
    // Could transfer zip crc32 and content over into a gzip file,
    // but typical use case is that starting with uncompressed data.
}

void ZipStream::close() {
    // do this to end the stream and cleanup
    KVERIFY(mz_deflateEnd(_stream.get()) == MZ_OK);
    
    // data is already all written, so just need the footer
    
    const uint32_t footer[2] = {
        _sourceCRC32,
        (uint32_t)(_sourceSize & 0xFFFFFFFF)
    };
    
    // gzip 8B trailer
    // 4b crc checksum of original data (can use mz_crc32())
    // 4b length of data (mod 0xFFFFFFFF), if bigger than 4gb then can only validate bottom 4B of length.
    bool success = _fileHelper.write((const uint8_t*)&footer, sizeof(footer));
    if (!success) {
        KLOGE("ZipStream", "Could not write gzip footer to %s", _fileHelper.filename().c_str());
    }
}

Slice ZipStream::write(const Slice& in) {
    // If in.size is huge, then don't resize like this.
    // But stream is assumed to take in smaller buffers
    // and know compressed stream is smaller than input size
    _compressed.resize(in.size());
    
    _stream->avail_in = in.size();
    _stream->next_in = in.data();
    
    // Have to set these up, since array may have grown
    _stream->avail_out = _compressed.size();
    _stream->next_out = _compressed.data();
    
    // Hope don't need to do this in a loop
    KVERIFY(mz_deflate(_stream.get(), MZ_FULL_FLUSH) == MZ_OK);
    
    // TODO: would be nice to skip crc32 work
    _sourceSize += in.size();
    mz_crc32(_sourceCRC32, in.data(), in.size());
    
    // return the compressed output
    int numBytesCompressed = _compressed.size() - _stream->avail_out;
    return Slice(_compressed.data(), numBytesCompressed);
}

void ZipStream::compress(const Slice& uncompressedData) {
    Slice compressedSlice = write(uncompressedData);
    
    // This writes out to a fileHelper
    _fileHelper.write(compressedSlice.data(), compressedSlice.size());
}



} // namespace kram
