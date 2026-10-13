/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "firstrunwizard.hpp"
#include "util/color_scheme_model.hpp"
#include "maininterface/mainctx.hpp"
#include "dialogs/toolbar/controlbar_profile_model.hpp"
#include "medialibrary/medialib.hpp"

#include <QPushButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_url.h>
#include <vlc_cxx_helpers.hpp>

namespace {

// Redesign: previews of the color schemes (see the "Onboarding" frames of the design)
QPixmap colorSchemePreview( ColorSchemeModel::ColorScheme scheme, qreal dpr )
{
    constexpr int size = 88;

    QPixmap pixmap( QSize( size, size ) * dpr );
    pixmap.setDevicePixelRatio( dpr );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setRenderHint( QPainter::Antialiasing );

    const QRectF rect( 0.5, 0.5, size - 1, size - 1 );
    QPainterPath shape;
    shape.addRoundedRect( rect, 10, 10 );

    const QColor light( "#DDDDDD" );
    const QColor dark( "#191919" );

    switch( scheme )
    {
        case ColorSchemeModel::Day:
            painter.fillPath( shape, light );
            break;
        case ColorSchemeModel::Night:
            painter.fillPath( shape, dark );
            break;
        default:
        {
            painter.fillPath( shape, dark );
            QPainterPath half;
            half.moveTo( rect.topLeft() );
            half.lineTo( rect.right() - rect.width() / 3, rect.top() );
            half.lineTo( rect.left() + rect.width() / 3, rect.bottom() );
            half.lineTo( rect.bottomLeft() );
            half.closeSubpath();
            painter.fillPath( shape.intersected( half ), light );
            break;
        }
    }

    painter.setPen( QPen( QColor( "#404040" ), 1 ) );
    painter.drawPath( shape );

    return pixmap;
}

QString prettyKeyName( const QString& key )
{
    if( key == QLatin1String( "Left" ) )
        return QStringLiteral( "\u2190" );
    if( key == QLatin1String( "Right" ) )
        return QStringLiteral( "\u2192" );
    if( key == QLatin1String( "Up" ) )
        return QStringLiteral( "\u2191" );
    if( key == QLatin1String( "Down" ) )
        return QStringLiteral( "\u2193" );
    if( key.size() == 1 )
        return key.toUpper();
    return key;
}

// Key caps of the first binding of a hotkey ("Ctrl+Shift+s\tMedia Play")
QWidget* keyCaps( const QString& bindings, QWidget* parent )
{
    QWidget* widget = new QWidget( parent );
    QHBoxLayout* layout = new QHBoxLayout( widget );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 4 );

    const QString binding = bindings.section( QLatin1Char( '\t' ), 0, 0 );

    QStringList keys;
    if( binding == QLatin1String( "+" ) )
        keys << binding;
    else if( binding.endsWith( QLatin1String( "++" ) ) )
        keys = binding.left( binding.size() - 2 ).split( QLatin1Char( '+' ) ) << QStringLiteral( "+" );
    else if( !binding.isEmpty() )
        keys = binding.split( QLatin1Char( '+' ) );

    if( keys.isEmpty() )
        keys << QStringLiteral( "\u2014" );

    for( const QString& key : keys )
    {
        QLabel* cap = new QLabel( prettyKeyName( key ), widget );
        cap->setObjectName( QStringLiteral( "wizardKeyCap" ) );
        cap->setAlignment( Qt::AlignCenter );
        layout->addWidget( cap );
    }
    layout->addStretch( 1 );

    return widget;
}

} // anonymous namespace

FirstRunWizard::FirstRunWizard( qt_intf_t *_p_intf, QWidget *parent)
               : QWizard( parent )
{
    p_intf = _p_intf;

    /* Set windown properties */
    setWindowTitle( qtr( "Welcome" ) );
    setWindowModality( Qt::WindowModal );

    // NOTE: Aero style seems to be incompatible with
    //       Qt Modern Windows Style dark mode
    setWizardStyle(QWizard::ModernStyle);

    /* Build the Ui */
    ui.setupUi( this );

    /* Set the privacy and network policy */
    ui.policy->setHtml( qtr( "<p>In order to protect your privacy, <i>VLC media player</i> "
        "does <b>not</b> collect personal data or transmit them, "
        "not even in anonymized form, to anyone."
        "</p>\n"
        "<p>Nevertheless, <i>VLC</i> is able to automatically retrieve "
        "information about the media in your play queue from third party "
        "Internet-based services. This includes cover art, track names, "
        "artist names and other meta-data."
        "</p>\n"
        "<p>Consequently, this may entail identifying some of your media files to third party "
        "entities. Therefore the <i>VLC</i> developers require your express "
        "consent for the media player to access the Internet automatically."
        "</p>\n" ) );
    ui.policy->setReadOnly( true );

    /* Set up the button group for colour schemes. */
    /* Creating in Qt Designer has unstable ordering when calling buttons() */
    colorSchemeGroup = new QButtonGroup( this );
    colorSchemeGroup->addButton( ui.systemButton );
    colorSchemeGroup->addButton( ui.lightButton );
    colorSchemeGroup->addButton( ui.darkButton );
    colorSchemeGroup->setExclusive( true );

    colorSchemeImages = new QButtonGroup( this );
    colorSchemeImages->addButton( ui.daynightImage );
    colorSchemeImages->addButton( ui.lightImage );
    colorSchemeImages->addButton( ui.darkImage );

    /* Setup the layout page */
    ui.layoutGroup->setId( ui.modernButton, 0 );
    ui.layoutGroup->setId( ui.classicButton, 1 );

    layoutImages = new QButtonGroup( this );
    layoutImages->addButton( ui.modernImage );
    layoutImages->addButton( ui.classicImage );
    layoutImages->setId( ui.modernImage, MODERN );
    layoutImages->setId( ui.classicImage, CLASSIC );

    /* Set the tooltips for each of the layout choices */
    ui.classicButton->setToolTip( qtr("<ul>"
                                      "<li>No client-side decoration</li>"
                                      "<li>Toolbar menu</li>"
                                      "<li>Pinned video controls</li>"
                                      "</ul>") );

    ui.modernButton->setToolTip( qtr("<ul>"
                                     "<li>Client-side decoration</li>"
                                     "<li>No toolbar menu</li>"
                                     "<li>Video controls unpinned</li>"
                                     "</ul>") );

    /* Remove the cancel button */
    setOption( QWizard::NoCancelButton );

    /* Create new instance of MLFoldersModel */
    if ( vlc_ml_instance_get( p_intf ) )
    {
        const auto foldersModel = new MLFoldersModel( this );
        foldersModel->setCtx( p_intf->p_mi );
        ui.entryPoints->setMLFoldersModel( foldersModel );
        mlFoldersEditor = ui.entryPoints;
        mlFoldersModel = foldersModel;
    }
    else
    {
        ui.enableMl->setChecked( false );
    }

    /* Disable the ML button as toggling doesn't actually do anything */
    ui.enableMl->setEnabled(false);

    /* Slots and Signals */
    connect( ui.addButton, &QPushButton::clicked, this, &FirstRunWizard::MLaddNewFolder );
    connect( colorSchemeGroup, qOverload<QAbstractButton*>( &QButtonGroup::buttonClicked ), this, &FirstRunWizard::updateColorLabel );
    connect( colorSchemeImages, qOverload<QAbstractButton*>( &QButtonGroup::buttonClicked ), this, &FirstRunWizard::imageColorSchemeClick );
    connect( ui.layoutGroup, qOverload<QAbstractButton*>( &QButtonGroup::buttonClicked ), this, &FirstRunWizard::updateLayoutLabel );
    connect( layoutImages, qOverload<QAbstractButton*>( &QButtonGroup::buttonClicked ), this, &FirstRunWizard::imageLayoutClick );
    connect( this->button(QWizard::FinishButton), &QPushButton::clicked, this, &FirstRunWizard::finish );

    setupRedesign();
}

/**
 * Redesign: applies the layout of the "Onboarding" frames of the design to
 * the pages of the wizard (their look is defined by the style sheet of the
 * application, see style/widgetstyle.cpp)
 */
void FirstRunWizard::setupRedesign()
{
    setMinimumSize( 760, 560 );
    setOption( QWizard::NoBackButtonOnStartPage );

    const qreal dpr = devicePixelRatioF();

    /* Welcome page: logo, title and description above the privacy policy */
    {
        QWidget* header = new QWidget( ui.welcomePage );
        QVBoxLayout* headerLayout = new QVBoxLayout( header );
        headerLayout->setSpacing( 12 );

        QLabel* logo = new QLabel( header );
        logo->setPixmap( QIcon( QStringLiteral( ":/logo/vlc128.png" ) ).pixmap( QSize( 72, 72 ) ) );
        logo->setAlignment( Qt::AlignHCenter );

        QLabel* title = new QLabel( qtr( "Welcome to VLC!" ), header );
        title->setObjectName( QStringLiteral( "wizardTitle" ) );
        title->setAlignment( Qt::AlignHCenter );

        QLabel* description = new QLabel( qtr( "VLC is a free and open source cross-platform multimedia player "
                                               "and framework that plays most multimedia files as well as DVDs, "
                                               "Audio CDs, VCDs, and various streaming protocols." ), header );
        description->setObjectName( QStringLiteral( "wizardDescription" ) );
        description->setAlignment( Qt::AlignHCenter );
        description->setWordWrap( true );

        headerLayout->addWidget( logo, 0, Qt::AlignHCenter );
        headerLayout->addWidget( title, 0, Qt::AlignHCenter );
        // NOTE: a word wrapped label must not be aligned in its layout,
        //       otherwise it gets a too small width (and height)
        headerLayout->addWidget( description );
        headerLayout->setContentsMargins( 60, 8, 60, 16 );

        ui.gridLayout->removeWidget( ui.horizontalFrame_policy );
        ui.gridLayout->removeWidget( ui.horizontalFrame_prefs );
        ui.gridLayout->addWidget( header, 0, 0 );
        ui.gridLayout->addWidget( ui.horizontalFrame_policy, 1, 0 );
        ui.gridLayout->addWidget( ui.horizontalFrame_prefs, 2, 0 );

        // The media library option can not be changed here, the folders are
        // set on the next page when the media library is available:
        ui.horizontalFrame_prefs->hide();
    }

    /* Color scheme page: centered title, scheme cards with their name below */
    {
        ui.gridLayout_3->removeWidget( ui.colorLabel );
        ui.gridLayout_3->addWidget( ui.colorLabel, 0, 0, 1, 3, Qt::AlignHCenter );
        ui.colorLabel->setText( qtr( "Select your preferred theme and color." ) );

        const struct {
            QVBoxLayout* layout;
            QAbstractButton* button;
            QPushButton* image;
            ColorSchemeModel::ColorScheme scheme;
        } cards[] = {
            { ui.verticalLayout_2, ui.systemButton, ui.daynightImage, ColorSchemeModel::System },
            { ui.verticalLayout_4, ui.lightButton, ui.lightImage, ColorSchemeModel::Day },
            { ui.verticalLayout_5, ui.darkButton, ui.darkImage, ColorSchemeModel::Night },
        };

        for( const auto& card : cards )
        {
            card.image->setAutoDefault( false );
            card.image->setIcon( colorSchemePreview( card.scheme, dpr ) );
            card.image->setIconSize( QSize( 88, 88 ) );
            card.image->setCheckable( true );
            card.image->setChecked( card.button->isChecked() );

            card.layout->removeWidget( card.button );
            card.layout->addWidget( card.button, 0, Qt::AlignHCenter );
            card.layout->setAlignment( card.image, Qt::AlignHCenter );
        }

        setupAccentColors();
    }

    /* Layout page */
    {
        ui.gridLayout_4->removeWidget( ui.layoutLabel );
        ui.gridLayout_4->addWidget( ui.layoutLabel, 0, 0, 1, 2, Qt::AlignHCenter );

        const struct {
            QVBoxLayout* layout;
            QAbstractButton* button;
            QPushButton* image;
        } cards[] = {
            { ui.verticalLayout_6, ui.modernButton, ui.modernImage },
            { ui.verticalLayout_7, ui.classicButton, ui.classicImage },
        };

        for( const auto& card : cards )
        {
            card.image->setAutoDefault( false );
            card.image->setCheckable( true );
            card.image->setChecked( card.button->isChecked() );
            // the selection border hugs the preview:
            card.image->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
            card.layout->setAlignment( card.image, Qt::AlignHCenter );

            card.layout->removeWidget( card.button );
            card.layout->addWidget( card.button, 0, Qt::AlignHCenter );
        }
    }

    /* Keyboard shortcuts page */
    setPage( SHORTCUTS_PAGE, createShortcutsPage() );
}

/**
 * Redesign: accent color swatches of the color scheme page
 */
void FirstRunWizard::setupAccentColors()
{
    // see the "Onboarding" frames of the design, the first one is the
    // default accent of the interface
    static const struct {
        const char* value;
        const char* color;
    } accents[] = {
        { "default", "#D37918" },
        { "#E15032", "#E15032" },
        { "#C49E23", "#C49E23" },
        { "#19A234", "#19A234" },
        { "#18AAD3", "#18AAD3" },
        { "#185DD3", "#185DD3" },
        { "#8E42FF", "#8E42FF" },
        { "#F052B3", "#F052B3" },
        { "#AFAFAF", "#AFAFAF" },
    };

    QString current;
    {
        char* value = config_GetPsz( "qt-accent-color" );
        current = value ? qfu( value ) : QStringLiteral( "default" );
        free( value );
    }

    QWidget* row = new QWidget( ui.colorSchemePage );
    QHBoxLayout* rowLayout = new QHBoxLayout( row );
    rowLayout->setContentsMargins( 0, 8, 0, 8 );
    rowLayout->setSpacing( 14 );

    accentGroup = new QButtonGroup( this );
    accentGroup->setExclusive( true );

    const QPalette& pal = palette();
    // the ring of the swatches is invisible on the page background:
    const QString background = pal.color( QPalette::Base ).name();
    const QString ring = pal.color( QPalette::Text ).name();

    bool checked = false;
    for( const auto& accent : accents )
    {
        QPushButton* swatch = new QPushButton( row );
        swatch->setObjectName( QStringLiteral( "wizardAccentSwatch" ) );
        swatch->setCheckable( true );
        // do not take the default button role from "Next" when clicked:
        swatch->setAutoDefault( false );
        swatch->setCursor( Qt::PointingHandCursor );
        swatch->setProperty( "accentValue", QString::fromLatin1( accent.value ) );
        swatch->setAccessibleName( QString::fromLatin1( accent.color ) );
        swatch->setStyleSheet( QStringLiteral(
            "QPushButton#wizardAccentSwatch {"
            "  background-color: %1;"
            "  border: 3px solid %2;"
            "  border-radius: 15px;"
            "  padding: 0;"
            // the style sheet sizes win over setFixedSize(): 26 + 2 * 3 (border) = 32
            "  min-width: 26px;"
            "  max-width: 26px;"
            "  min-height: 26px;"
            "  max-height: 26px;"
            "}"
            "QPushButton#wizardAccentSwatch:hover {"
            "  border-color: %3;"
            "}"
            "QPushButton#wizardAccentSwatch:checked {"
            "  border-color: %4;"
            "}" ).arg( QString::fromLatin1( accent.color ), background,
                       pal.color( QPalette::Mid ).name(), ring ) );

        if( !checked && current.compare( QString::fromLatin1( accent.value ), Qt::CaseInsensitive ) == 0 )
        {
            swatch->setChecked( true );
            checked = true;
        }

        accentGroup->addButton( swatch );
        rowLayout->addWidget( swatch );
    }

    connect( accentGroup, qOverload<QAbstractButton*>( &QButtonGroup::buttonClicked ),
             this, &FirstRunWizard::accentClicked );

    ui.gridLayout_3->addWidget( row, 6, 0, 1, 3, Qt::AlignHCenter );
}

void FirstRunWizard::accentClicked( QAbstractButton* button )
{
    // the interface follows the new accent immediately:
    p_intf->p_mi->setAccentColor( button->property( "accentValue" ).toString() );
    accentChanged = true;
}

/**
 * Redesign: last page, the basic keyboard shortcuts
 */
QWizardPage* FirstRunWizard::createShortcutsPage()
{
    QWizardPage* page = new QWizardPage( this );
    QVBoxLayout* layout = new QVBoxLayout( page );
    layout->setSpacing( 0 );

    QLabel* title = new QLabel( qtr( "Learn the basic keyboard shortcuts." ), page );
    title->setObjectName( QStringLiteral( "wizardTitle" ) );
    title->setAlignment( Qt::AlignHCenter );
    layout->addSpacing( 16 );
    layout->addWidget( title );
    layout->addSpacing( 28 );

    static const char* const hotkeys[] = {
        "key-play-pause",
        "key-toggle-fullscreen",
        "key-jump-extrashort",
        "key-jump+extrashort",
        "key-vol-up",
        "key-vol-down",
        "key-prev",
        "key-next",
        "key-vol-mute",
        "key-snapshot",
        "key-audio-track",
        "key-subtitle-track",
    };

    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing( 16 );
    grid->setVerticalSpacing( 14 );

    int index = 0;
    for( const char* hotkey : hotkeys )
    {
        const module_config_t* item = config_FindConfig( hotkey );
        if( !item || !item->psz_text )
            continue;

        char* value = config_GetPsz( hotkey );
        const QString binding = value ? qfu( value ) : QString();
        free( value );

        QLabel* label = new QLabel( qfut( item->psz_text ), page );
        label->setObjectName( QStringLiteral( "wizardShortcutLabel" ) );

        const int row = index / 2;
        const int column = ( index % 2 ) * 3;
        grid->addWidget( label, row, column );
        grid->addWidget( keyCaps( binding, page ), row, column + 1 );
        ++index;
    }
    grid->setColumnStretch( 0, 2 );
    grid->setColumnStretch( 1, 1 );
    grid->setColumnMinimumWidth( 2, 24 );
    grid->setColumnStretch( 3, 2 );
    grid->setColumnStretch( 4, 1 );

    layout->addLayout( grid );
    layout->addStretch( 1 );

    QLabel* hint = new QLabel( qtr( "You can change the shortcuts in the preferences." ), page );
    hint->setObjectName( QStringLiteral( "wizardHint" ) );
    layout->addWidget( hint );

    return page;
}

/**
 * Function called when the finish button is pressed.
 * Processes all inputted settings according to chosen options
 */
void FirstRunWizard::finish()
{
    /* Welcome Page settings  */
    config_PutInt( "metadata-network-access", ui.privacyCheckbox->isChecked() );
    config_PutInt( "qt-privacy-ask", 0 );

    /* Colour Page settings */
    p_intf->p_mi->getColorScheme()->setCurrentIndex( colorSchemeGroup->checkedId() );

    /* Layout Page settings */
    config_PutInt( "qt-menubar", ui.layoutGroup->checkedId() );
    config_PutInt( "qt-titlebar", ui.layoutGroup->checkedId() );

    config_PutInt( "qt-pin-controls", ui.layoutGroup->checkedId() );

    {
        ControlbarProfileModel* controlbarModel = p_intf->p_mi->controlbarProfileModel();
        assert(controlbarModel);

        ControlbarProfileModel::Style style;
        if( ui.layoutGroup->checkedId() )
            style = ControlbarProfileModel::Style::CLASSIC_STYLE;
        else
            style = ControlbarProfileModel::Style::REDESIGN_STYLE;

        std::unique_ptr<ControlbarProfile> profile( controlbarModel->generateProfileFromStyle( style ) );
        assert( profile );
        const std::optional<int> index = controlbarModel->findModel( profile.get() );
        if( index )
            controlbarModel->setSelectedProfile( *index );
        else
            controlbarModel->insertProfile( std::move( profile ), true );
    }

    /* Commit changes to the scanned folders for the Media Library */
    if( vlc_ml_instance_get( p_intf ) && mlFoldersEditor )
        mlFoldersEditor->commit();

    /* Reload the indexing service for the media library */
    if( p_intf->p_mi->getMediaLibrary() )
        p_intf->p_mi->getMediaLibrary()->reload();

    p_intf->p_mi->reloadPrefs();
    p_intf->p_mi->controlbarProfileModel()->save();
    config_SaveConfigFile( p_intf );
}

/**
 * Opens a QFileDialog for the user to select a folder to add to the Media Library
 * If a folder is selected, add it to the list of folders under MLFoldersEditor
 */
void FirstRunWizard::MLaddNewFolder()
{
    QUrl newEntryPoint = QFileDialog::getExistingDirectoryUrl( this, qtr("Choose a folder to add to the Media Library"),
                                                               QUrl( QDir::homePath() ));

    if( !newEntryPoint.isEmpty() && !mlFoldersEditor->contains( newEntryPoint ) )
        mlFoldersEditor->add( newEntryPoint );
}

/**
 * Automatically updates the label on the color scheme page depending
 * on the currently selected radio button choice
 * @param id The id of the button we are updating the label of
 */
void FirstRunWizard::updateColorLabel( QAbstractButton* btn )
{
    /* Redesign: the preview of the selected scheme is highlighted */
    if( QAbstractButton* image = colorSchemeImages->button( colorSchemeGroup->id( btn ) ) )
        image->setChecked( true );

    switch ( colorSchemeGroup->id(btn) )
    {
        case ColorSchemeModel::System:
            ui.explainerLabel->setText( qtr( "<i>VLC will automatically switch to dark mode accordingly with system settings</i>" ) );
            break;
        case ColorSchemeModel::Day:
            ui.explainerLabel->setText( qtr( "<i>VLC will automatically use light mode</i>" ) );
            break;
        case ColorSchemeModel::Night:
            ui.explainerLabel->setText( qtr( "<i>VLC will automatically use dark mode</i>" ) );
            break;
    }
}

/**
 * Automatically updates the label depending on the currently selected layout
 * @param id The id of the button we are updating the label of
 */
void FirstRunWizard::updateLayoutLabel( QAbstractButton* btn )
{
    /* Redesign: the preview of the selected layout is highlighted */
    if( QAbstractButton* image = layoutImages->button( ui.layoutGroup->id( btn ) ) )
        image->setChecked( true );

    switch ( ui.layoutGroup->id( btn ) )
    {
        case MODERN:
            ui.layoutExplainer->setText( qtr( "<i>VLC will use a modern layout with no menubar or pinned controls but with client-side decoration</i>" ) );
            break;
        case CLASSIC:
            ui.layoutExplainer->setText( qtr( "<i>VLC will use a classic layout with a menubar and pinned controls but with no client-side decoration</i>" ) );
            break;
    }
}

/**
 * Checks the correct button when the corresponding image is clicked
 * for the color scheme page.
 * @param id The id of the image that was clicked
 */
void FirstRunWizard::imageColorSchemeClick( QAbstractButton* btn )
{
    QAbstractButton* groupBtn = colorSchemeGroup->buttons().at( colorSchemeImages->id( btn ) );
    assert( groupBtn );
    groupBtn->setChecked( true );
    updateColorLabel( groupBtn );
}

/**
 * Checks the correct button when the corresponding image is clicked
 * for the layouts page.
 * @param id The id of the image that was clicked
 */
void FirstRunWizard::imageLayoutClick( QAbstractButton* btn )
{
    QAbstractButton* layoutBtn = ui.layoutGroup->buttons().at( layoutImages->id( btn ) );
    assert( layoutBtn );
    layoutBtn->setChecked( true );
    updateLayoutLabel( layoutBtn );
}

/**
 * Defines the navigation of pages for the wizard
 * @return int - the page id to go to or -1 if we are done
 */
int FirstRunWizard::nextId() const
{
    switch ( currentId() )
    {
        case WELCOME_PAGE:
            if( ui.enableMl->isChecked() )
                return FOLDER_PAGE;
            else
                return COLOR_SCHEME_PAGE;
        case FOLDER_PAGE:
            return COLOR_SCHEME_PAGE;
        case COLOR_SCHEME_PAGE:
            return LAYOUT_PAGE;
        case LAYOUT_PAGE:
            return SHORTCUTS_PAGE;
        default:
            return -1;
    }
}

/**
 * Sets up the buttons and options for the color scheme page
 * Needs to be set up later or else the main interface hasn't been created yet
 * Color schemes are always in the order system/auto, day then night
 */
void FirstRunWizard::initializePage( int id )
{
    if(id == COLOR_SCHEME_PAGE)
    {
        QVector<ColorSchemeModel::Item> schemes = p_intf->p_mi->getColorScheme()->getSchemes();
        auto schemeButtons = colorSchemeGroup->buttons();
        auto schemeImages = colorSchemeImages->buttons();

        /* Number of buttons should be equal to the schemes we can choose from */
        assert( schemes.size() == schemeButtons.size() );
        assert( schemes.size() == schemeImages.size() );

        for( int i = 0; i < schemes.size(); i++ )
        {
            colorSchemeGroup->setId( schemeButtons.at(i), schemes.at(i).scheme );
            colorSchemeImages->setId( schemeImages.at(i), schemes.at(i).scheme );
            schemeButtons.at(i)->setText( schemes.at(i).text );
            if( !i ) updateColorLabel( schemeButtons.at(i) );
        }
    }
    else if ( id == FOLDER_PAGE )
        addDefaults();
}

/**
 * Processes the default options on rejection of the FirstRun Wizard.
 * The default options are:
 * - Yes to metadata
 * - System/Auto colour scheme
 * - Modern VLC layout
 */
void FirstRunWizard::reject()
{
    assert(p_intf->p_mi);
    /* Welcome Page settings  */
    config_PutInt( "metadata-network-access", 1 );
    config_PutInt( "qt-privacy-ask", 0 );

    /* Colour Page settings */
    p_intf->p_mi->getColorScheme()->setCurrentIndex( 0 );
    if( accentChanged )
        p_intf->p_mi->setAccentColor( QStringLiteral( "default" ) );

    /* Layout Page settings */
    config_PutInt( "qt-menubar", 0 );
    config_PutInt( "qt-titlebar", 0 );
    p_intf->p_mi->setPinVideoControls( 0 );

    if( p_intf->p_mi->getMediaLibrary() )
        p_intf->p_mi->getMediaLibrary()->reload();

    config_SaveConfigFile( p_intf );
    p_intf->p_mi->reloadPrefs();
    p_intf->p_mi->controlbarProfileModel()->save();

    done( QDialog::Rejected );
}

/**
 * Adds the default folders to the media library
 */
void FirstRunWizard::addDefaults()
{
    // Return if we already set the defaults or something is null
    if( mlDefaults || !mlFoldersEditor || mlFoldersEditor->rowCount() )
        return;

    for( auto&& target : { VLC_VIDEOS_DIR, VLC_MUSIC_DIR } )
    {
        auto folder = vlc::wrap_cptr( config_GetUserDir( target ) );
        if( folder == nullptr )
            continue;
        auto folderMrl = vlc::wrap_cptr( vlc_path2uri( folder.get(), nullptr ) );
        if ( !mlFoldersEditor->contains( QUrl( folderMrl.get() ) ) )
            mlFoldersEditor->add( QUrl( folderMrl.get() ) );
    }

    mlDefaults = true;
}
