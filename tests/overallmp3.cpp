#include "./overall.h"

#include "../abstracttrack.h"
#include "../mpegaudio/mpegaudioframe.h"
#include "../id3/id3v1tag.h"
#include "../id3/id3v2tag.h"

/*!
 * \brief Checks "mtx-test-data/mp3/id3-tag-and-xing-header.mp3"
 */
void OverallTests::checkMp3Testfile1()
{
    CPPUNIT_ASSERT(m_fileInfo.containerFormat() == ContainerFormat::MpegAudioFrames);
    const auto tracks = m_fileInfo.tracks();
    CPPUNIT_ASSERT(tracks.size() == 1);
    for(const auto &track : tracks) {
        CPPUNIT_ASSERT(track->mediaType() == MediaType::Audio);
        CPPUNIT_ASSERT(track->format() == GeneralMediaFormat::Mpeg1Audio);
        CPPUNIT_ASSERT(track->format().sub == SubFormats::Mpeg1Layer3);
        CPPUNIT_ASSERT(track->channelCount() == 2);
        CPPUNIT_ASSERT(track->channelConfig() == static_cast<byte>(MpegChannelMode::JointStereo));
        CPPUNIT_ASSERT(track->samplingFrequency() == 44100);
        CPPUNIT_ASSERT(track->duration().seconds() == 3);
    }
    const auto tags = m_fileInfo.tags();
    switch(m_tagStatus) {
    case TagStatus::Original:
        CPPUNIT_ASSERT(m_fileInfo.id3v1Tag());
        CPPUNIT_ASSERT(m_fileInfo.id3v2Tags().size() == 1);
        CPPUNIT_ASSERT(tags.size() == 2);
        for(const auto &tag : tags) {
            CPPUNIT_ASSERT(tag->value(KnownField::TrackPosition).toPositionInSet().position() == 4);
            CPPUNIT_ASSERT(tag->value(KnownField::Year).toString() == "1984");
            switch(tag->type()) {
            case TagType::Id3v1Tag:
                CPPUNIT_ASSERT(tag->value(KnownField::Title).toString() == "Cohesion");
                CPPUNIT_ASSERT(tag->value(KnownField::Artist).toString() == "Minutemen");
                CPPUNIT_ASSERT(tag->value(KnownField::Album).toString() == "Double Nickels On The Dime");
                CPPUNIT_ASSERT(tag->value(KnownField::Genre).toString() == "Punk Rock");
                CPPUNIT_ASSERT(tag->value(KnownField::Comment).toString() == "ExactAudioCopy v0.95b4");
                break;
            case TagType::Id3v2Tag:
                CPPUNIT_ASSERT(tag->value(KnownField::Title).dataEncoding() == TagTextEncoding::Utf16LittleEndian);
                CPPUNIT_ASSERT(tag->value(KnownField::Title).toWString() == u"Cohesion");
                CPPUNIT_ASSERT(tag->value(KnownField::Title).toString(TagTextEncoding::Utf8) == "Cohesion");
                CPPUNIT_ASSERT(tag->value(KnownField::Artist).toWString() == u"Minutemen");
                CPPUNIT_ASSERT(tag->value(KnownField::Artist).toString(TagTextEncoding::Utf8) == "Minutemen");
                CPPUNIT_ASSERT(tag->value(KnownField::Album).toWString() == u"Double Nickels On The Dime");
                CPPUNIT_ASSERT(tag->value(KnownField::Album).toString(TagTextEncoding::Utf8) == "Double Nickels On The Dime");
                CPPUNIT_ASSERT(tag->value(KnownField::Genre).toWString() == u"Punk Rock");
                CPPUNIT_ASSERT(tag->value(KnownField::Genre).toString(TagTextEncoding::Utf8) == "Punk Rock");
                CPPUNIT_ASSERT(tag->value(KnownField::Comment).toWString() == u"ExactAudioCopy v0.95b4");
                CPPUNIT_ASSERT(tag->value(KnownField::Comment).toString(TagTextEncoding::Utf8) == "ExactAudioCopy v0.95b4");
                CPPUNIT_ASSERT(tag->value(KnownField::TrackPosition).toPositionInSet().total() == 43);
                CPPUNIT_ASSERT(tag->value(KnownField::Length).toTimeSpan().isNull());
                CPPUNIT_ASSERT(tag->value(KnownField::Lyricist).isEmpty());
                break;
            default:
                ;
            }
        }
        break;
    case TagStatus::TestMetaDataPresent:
        checkMp3TestMetaData();
        break;
    case TagStatus::Removed:
        CPPUNIT_ASSERT(tags.size() == 0);
    }

}

/*!
 * \brief Checks whether test meta data for MP3 files has been applied correctly.
 */
void OverallTests::checkMp3TestMetaData()
{
    using namespace Mp3TestFlags;

    // check whether tags are assigned according to the current test mode
    Id3v1Tag *id3v1Tag = nullptr;
    Id3v2Tag *id3v2Tag = nullptr;
    if(m_mode & Id3v2AndId3v1) {
        CPPUNIT_ASSERT(id3v1Tag = m_fileInfo.id3v1Tag());
        CPPUNIT_ASSERT(id3v2Tag = m_fileInfo.id3v2Tags().at(0).get());
    } else if(m_mode & Id3v1Only) {
        CPPUNIT_ASSERT(id3v1Tag = m_fileInfo.id3v1Tag());
        CPPUNIT_ASSERT(m_fileInfo.id3v2Tags().empty());
    } else {
        CPPUNIT_ASSERT(!m_fileInfo.id3v1Tag());
        CPPUNIT_ASSERT(id3v2Tag = m_fileInfo.id3v2Tags().at(0).get());
    }

    // check common test meta data
    for(Tag *tag : initializer_list<Tag *>{id3v1Tag, id3v2Tag}) {
        if(tag) {
            CPPUNIT_ASSERT(tag->value(KnownField::Title) == m_testTitle);
            CPPUNIT_ASSERT(tag->value(KnownField::Comment) == m_testComment);
            CPPUNIT_ASSERT(tag->value(KnownField::Album) == m_testAlbum);
            CPPUNIT_ASSERT(tag->value(KnownField::Artist) == m_preservedMetaData.front());
            // TODO: check more fields
            m_preservedMetaData.pop();
        }
    }
    // test ID3v1 specific test meta data
    if(id3v1Tag) {
        CPPUNIT_ASSERT(id3v1Tag->value(KnownField::TrackPosition).toPositionInSet().position() == m_testPosition.toPositionInSet().position());
    }
    // test ID3v2 specific test meta data
    if(id3v2Tag) {
        CPPUNIT_ASSERT(id3v2Tag->value(KnownField::TrackPosition) == m_testPosition);
        CPPUNIT_ASSERT(id3v2Tag->value(KnownField::DiskPosition) == m_testPosition);
    }
}

/*!
 * \brief Checks whether padding constraints are met.
 */
void OverallTests::checkMp3PaddingConstraints()
{
    using namespace Mp3TestFlags;

    if(!(m_mode & Id3v1Only)) {
        if(m_mode & PaddingConstraints) {
            if(m_mode & ForceRewring) {
                CPPUNIT_ASSERT(m_fileInfo.paddingSize() == 4096);
            } else {
                CPPUNIT_ASSERT(m_fileInfo.paddingSize() >= 1024);
                CPPUNIT_ASSERT(m_fileInfo.paddingSize() <= (4096 + 1024));
            }
        }
    } else {
        // adding padding is not possible if no ID3v2 tag is present
    }
    // TODO: check rewriting behaviour
}

void OverallTests::setMp3TestMetaData()
{
    using namespace Mp3TestFlags;

    // ensure tags are assigned according to the current test mode
    Id3v1Tag *id3v1Tag = nullptr;
    Id3v2Tag *id3v2Tag = nullptr;
    if(m_mode & Id3v2AndId3v1) {
        id3v1Tag = m_fileInfo.createId3v1Tag();
        id3v2Tag = m_fileInfo.createId3v2Tag();
    } else if(m_mode & Id3v1Only) {
        id3v1Tag = m_fileInfo.createId3v1Tag();
        m_fileInfo.removeAllId3v2Tags();
    } else {
        m_fileInfo.removeId3v1Tag();
        id3v2Tag = m_fileInfo.createId3v2Tag();
    }

    // assign some test meta data
    for(Tag *tag : initializer_list<Tag *>{id3v1Tag, id3v2Tag}) {
        if(tag) {
            tag->setValue(KnownField::Title, m_testTitle);
            tag->setValue(KnownField::Comment, m_testComment);
            tag->setValue(KnownField::Album, m_testAlbum);
            m_preservedMetaData.push(tag->value(KnownField::Artist));
            tag->setValue(KnownField::TrackPosition, m_testPosition);
            tag->setValue(KnownField::DiskPosition, m_testPosition);
            // TODO: set more fields
        }
    }
}

/*!
 * \brief Tests the MP3 parser via MediaFileInfo.
 */
void OverallTests::testMp3Parsing()
{
    cerr << endl << "MP3 parser" << endl;
    m_fileInfo.setForceFullParse(false);
    m_tagStatus = TagStatus::Original;
    parseFile(TestUtilities::testFilePath("mtx-test-data/mp3/id3-tag-and-xing-header.mp3"), &OverallTests::checkMp3Testfile1);
}

#ifdef PLATFORM_UNIX
/*!
 * \brief Tests the MP3 maker via MediaFileInfo.
 * \remarks Relies on the parser to check results.
 */
void OverallTests::testMp3Making()
{
    // full parse is required to determine padding
    m_fileInfo.setForceFullParse(true);

    // do the test under different conditions
    for(m_mode = 0; m_mode != 0x10; ++m_mode) {
        using namespace Mp3TestFlags;

        // setup test conditions
        m_fileInfo.setForceRewrite(m_mode & ForceRewring);
        m_fileInfo.setTagPosition(ElementPosition::Keep);
        m_fileInfo.setIndexPosition(ElementPosition::Keep);
        m_fileInfo.setPreferredPadding(m_mode & PaddingConstraints ? 4096 : 0);
        m_fileInfo.setMinPadding(m_mode & PaddingConstraints ? 1024 : 0);
        m_fileInfo.setMaxPadding(m_mode & PaddingConstraints ? (4096 + 1024) : static_cast<size_t>(-1));
        m_fileInfo.setForceTagPosition(false);
        m_fileInfo.setForceIndexPosition(false);

        // print test conditions
        list<string> testConditions;
        if(m_mode & ForceRewring) {
            testConditions.emplace_back("forcing rewrite");
        }
        if(m_mode & Id3v2AndId3v1) {
            if(m_mode & RemoveTag) {
                testConditions.emplace_back("removing tag");
            } else {
                testConditions.emplace_back("ID3v1 and ID3v2");
            }
        } else if(m_mode & Id3v1Only) {
            testConditions.emplace_back("ID3v1 only");
        } else {
            testConditions.emplace_back("ID3v2 only");
        }
        if(m_mode & PaddingConstraints) {
            testConditions.emplace_back("padding constraints");
        }
        cerr << endl << "MP3 maker - testmode " << m_mode << ": " << joinStrings(testConditions, ", ") << endl;

        // do actual tests
        m_tagStatus = (m_mode & RemoveTag) ? TagStatus::Removed : TagStatus::TestMetaDataPresent;
        void (OverallTests::*modifyRoutine)(void) = (m_mode & RemoveTag) ? &OverallTests::removeAllTags : &OverallTests::setMp3TestMetaData;
        makeFile(TestUtilities::workingCopyPath("mtx-test-data/mp3/id3-tag-and-xing-header.mp3"), modifyRoutine, &OverallTests::checkMp3Testfile1);
    }
}
#endif
