/** \file
 *  Game Develop
 *  2008-2012 Florian Rival (Florian.Rival@gmail.com)
 */
#ifndef AUTOMATISMMETADATA_H
#define AUTOMATISMMETADATA_H
#include <string>
#include <wx/bitmap.h>

namespace gd
{

/**
 * \brief Contains user-friendly information about an automatism type
 *
 * Implementations may derive from this class so as to provide more complete metadata if needed.
 * ( For example, Game Develop C++ Platform is shared pointers to objects that will be cloned so as to create the automatisms... )
 *
 * \ingroup Events
 */
class AutomatismMetadata
{
public:
    AutomatismMetadata() {};
    virtual ~AutomatismMetadata() {};

    AutomatismMetadata & SetFullName(const std::string & fullname_) { fullname = fullname_; return *this; }
    AutomatismMetadata & SetDefaultName(const std::string & defaultName_) { defaultName = defaultName_; return *this; }
    AutomatismMetadata & SetDescription(const std::string & description_) { description = description_; return *this; }
    AutomatismMetadata & SetGroup(const std::string & group_) { group = group_; return *this; }
    AutomatismMetadata & SetBitmapIcon(const wxBitmap & bitmap_) { icon = bitmap_; return *this; }

    const std::string & GetFullName() const { return fullname; }
    const std::string & GetDefaultName() const { return defaultName; }
    const std::string & GetDescription() const  { return description; }
    const std::string & GetGroup() const  { return group; }
    const wxBitmap & GetBitmapIcon() const { return icon; }

private:
    std::string fullname;
    std::string defaultName;
    std::string description;
    std::string group;
    wxBitmap icon;
};

}

#endif // AUTOMATISMMETADATA_H