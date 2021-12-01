#include "io/pboheaderreader.h"
#include <QTemporaryFile>
#include <gtest/gtest.h>
#include "io/pbofile.h"
#include "io/pboheaderio.h"
#include "io/pbofileformatexception.h"

namespace pboman3::io::test {
    TEST(PboHeaderReaderTest, ReadFileHeader_Reads_File_Without_Headers_Without_Signature) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();

        PboFile p(t.fileName());
        p.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&p);
        const PboNodeEntity e1("f1", PboPackingMethod::Packed, 0x01010101, 0x02020202,
                          0x03030303, 0x04040404);
        const PboNodeEntity e2 = PboNodeEntity::makeBoundary();

        io.writeEntry(e1);
        io.writeEntry(e2);

        p.close();
        t.close();

        //call the method
        p.open(QIODeviceBase::ReadOnly);
        const PboFileHeader header = PboHeaderReader::readFileHeader(&p);
        p.close();

        //verify the results
        ASSERT_EQ(0, header.headers.count());
        ASSERT_EQ(1, header.entries.count());

        ASSERT_EQ(header.entries.at(0)->fileName(), "f1");
        ASSERT_EQ(header.entries.at(0)->packingMethod(), PboPackingMethod::Packed);
        ASSERT_EQ(header.entries.at(0)->originalSize(), 0x01010101);
        ASSERT_EQ(header.entries.at(0)->reserved(), 0x02020202);
        ASSERT_EQ(header.entries.at(0)->timestamp(), 0x03030303);
        ASSERT_EQ(header.entries.at(0)->dataSize(), 0x04040404);

        ASSERT_EQ(header.dataBlockStart, 44);

        ASSERT_EQ(header.signature.size(), 0);
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Reads_File_Without_Headers_With_Signature) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();

        PboFile p(t.fileName());
        p.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&p);
        const PboNodeEntity e1("f1", PboPackingMethod::Packed, 0x01010101, 0x02020202,
                          0x03030303, 10);
        const PboNodeEntity e2 = PboNodeEntity::makeBoundary();
        const QByteArray signature(20, 5);

        io.writeEntry(e1);
        io.writeEntry(e2);
        p.write(QByteArray(e1.dataSize(), 1));
        p.write(QByteArray(1, 0)); //zero byte between data and signature
        p.write(signature);

        p.close();
        t.close();

        //call the method
        p.open(QIODeviceBase::ReadOnly);
        const PboFileHeader header = PboHeaderReader::readFileHeader(&p);
        p.close();

        //verify the results
        ASSERT_EQ(0, header.headers.count());
        ASSERT_EQ(1, header.entries.count());

        ASSERT_EQ(header.entries.at(0)->fileName(), "f1");
        ASSERT_EQ(header.entries.at(0)->packingMethod(), PboPackingMethod::Packed);
        ASSERT_EQ(header.entries.at(0)->originalSize(), 0x01010101);
        ASSERT_EQ(header.entries.at(0)->reserved(), 0x02020202);
        ASSERT_EQ(header.entries.at(0)->timestamp(), 0x03030303);
        ASSERT_EQ(header.entries.at(0)->dataSize(), 10);

        ASSERT_EQ(header.dataBlockStart, 44);

        ASSERT_EQ(header.signature, signature);
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Reads_File_With_Headers_With_Signature) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();

        PboFile p(t.fileName());
        p.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&p);

        const PboNodeEntity e0 = PboNodeEntity::makeSignature();
        const PboNodeEntity e1("f1", PboPackingMethod::Packed, 0x01010101, 0x02020202,
                          0x03030303, 5);
        const PboNodeEntity e2("f2", PboPackingMethod::Uncompressed, 0x05050505, 0x06060606,
                          0x07070707, 10);
        const PboNodeEntity e3 = PboNodeEntity::makeBoundary();

        const PboHeaderEntity h1("p1", "v1");
        const PboHeaderEntity h2("p2", "v2");
        const PboHeaderEntity h3 = PboHeaderEntity::makeBoundary();

        const QByteArray signature(20, 5);

        io.writeEntry(e0);
        io.writeHeader(h1);
        io.writeHeader(h2);
        io.writeHeader(h3);
        io.writeEntry(e1);
        io.writeEntry(e2);
        io.writeEntry(e3);
        p.write(QByteArray(e1.dataSize(), 1));
        p.write(QByteArray(e2.dataSize(), 2));
        p.write(QByteArray(1, 0)); //zero byte between data and signature
        p.write(signature);

        p.close();
        t.close();

        //call the method
        PboFile r(t.fileName());
        r.open(QIODeviceBase::ReadOnly);
        const PboFileHeader header = PboHeaderReader::readFileHeader(&r);
        r.close();

        //verify the results
        ASSERT_EQ(2, header.headers.count());
        ASSERT_EQ(header.headers.at(0)->name, "p1");
        ASSERT_EQ(header.headers.at(0)->value, "v1");
        ASSERT_EQ(header.headers.at(1)->name, "p2");
        ASSERT_EQ(header.headers.at(1)->value, "v2");

        ASSERT_EQ(2, header.entries.count());
        ASSERT_EQ(header.entries.at(0)->fileName(), "f1");
        ASSERT_EQ(header.entries.at(0)->packingMethod(), PboPackingMethod::Packed);
        ASSERT_EQ(header.entries.at(0)->originalSize(), 0x01010101);
        ASSERT_EQ(header.entries.at(0)->reserved(), 0x02020202);
        ASSERT_EQ(header.entries.at(0)->timestamp(), 0x03030303);
        ASSERT_EQ(header.entries.at(0)->dataSize(), 5);
        ASSERT_EQ(header.entries.at(1)->fileName(), "f2");
        ASSERT_EQ(header.entries.at(1)->packingMethod(), PboPackingMethod::Uncompressed);
        ASSERT_EQ(header.entries.at(1)->originalSize(), 0x05050505);
        ASSERT_EQ(header.entries.at(1)->reserved(), 0x06060606);
        ASSERT_EQ(header.entries.at(1)->timestamp(), 0x07070707);
        ASSERT_EQ(header.entries.at(1)->dataSize(), 10);

        ASSERT_EQ(header.dataBlockStart, 101);

        ASSERT_EQ(header.signature, signature);
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Reads_File_With_Broken_Signature) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();

        PboFile p(t.fileName());
        p.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&p);
        const PboNodeEntity e1("f1", PboPackingMethod::Packed, 0x01010101, 0x02020202,
                          0x03030303, 10);
        const PboNodeEntity e2 = PboNodeEntity::makeBoundary();

        io.writeEntry(e1);
        io.writeEntry(e2);
        p.write(QByteArray(e1.dataSize(), 1));
        p.write(QByteArray(1, 0)); //zero byte between data and signature
        p.write(QByteArray(15, 5)); //some junk to small to be a signature

        p.close();
        t.close();

        //call the method
        p.open(QIODeviceBase::ReadOnly);
        const PboFileHeader header = PboHeaderReader::readFileHeader(&p);
        p.close();

        //verify the results
        ASSERT_EQ(0, header.headers.count());
        ASSERT_EQ(1, header.entries.count());

        ASSERT_EQ(header.dataBlockStart, 44);

        ASSERT_EQ(header.signature.size(), 0);
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Throws_If_File_Is_Zero_Bytes) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();
        t.close();

        //call the method
        PboFile p(t.fileName());
        p.open(QIODeviceBase::ReadOnly);
        ASSERT_THROW(PboHeaderReader::readFileHeader(&p), PboFileFormatException);
        p.close();
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Throws_If_Starting_Entry_Corrupted) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();
        t.write(QByteArray(100, 0));
        t.close();

        //call the method
        PboFile p(t.fileName());
        p.open(QIODeviceBase::ReadOnly);
        ASSERT_THROW(PboHeaderReader::readFileHeader(&p), PboFileFormatException);
        p.close();
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Throws_If_Headers_Corrupted) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();

        PboFile p(t.fileName());
        p.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&p);
        io.writeEntry(PboNodeEntity::makeSignature());
        io.writeHeader(PboHeaderEntity("p1", "v1"));
        p.close();
        t.close();

        //call the method
        PboFile r(t.fileName());
        r.open(QIODeviceBase::ReadOnly);
        ASSERT_THROW(PboHeaderReader::readFileHeader(&r), PboFileFormatException);
        r.close();
    }

    TEST(PboHeaderReaderTest, ReadFileHeader_Throws_If_Entries_List_Corrupted) {
        //build a mock pbo file
        QTemporaryFile t;
        t.open();

        PboFile p(t.fileName());
        p.open(QIODeviceBase::WriteOnly);
        const PboHeaderIO io(&p);

        const PboNodeEntity e0 = PboNodeEntity::makeSignature();
        const PboNodeEntity e1("f1", PboPackingMethod::Packed, 0x01010101, 0x02020202,
                          0x03030303, 0x04040404);

        const PboHeaderEntity h1("p1", "v1");
        const PboHeaderEntity h2 = PboHeaderEntity::makeBoundary();

        io.writeEntry(e0);
        io.writeHeader(h1);
        io.writeHeader(h2);
        io.writeEntry(e1);
        //no boundary entry here, so the file is corrupted
        p.close();
        t.close();

        //call the method
        PboFile r(t.fileName());
        r.open(QIODeviceBase::ReadOnly);
        ASSERT_THROW(PboHeaderReader::readFileHeader(&r), PboFileFormatException);
        r.close();
    }
}
