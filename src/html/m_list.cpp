/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/m_list.cpp
// Purpose:     wxHtml module for lists
// Author:      Vaclav Slavik
// Copyright:   (c) 1999 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"


#if wxUSE_HTML && wxUSE_STREAMS

#ifndef WX_PRECOMP
    #include "wx/brush.h"
    #include "wx/dc.h"
#endif

#include "wx/html/forcelnk.h"
#include "wx/html/m_templ.h"

#include "wx/html/htmlcell.h"

FORCE_LINK_ME(m_list)


//-----------------------------------------------------------------------------
// wxHtmlListmarkCell
//-----------------------------------------------------------------------------

class wxHtmlListmarkCell : public wxHtmlCell
{
    private:
        wxBrush m_Brush;
    public:
        wxHtmlListmarkCell(wxDC *dc, const wxColour& clr);
        void Draw(wxDC& dc, int x, int y, int view_y1, int view_y2,
                  wxHtmlRenderingInfo& info) override;

    wxDECLARE_NO_COPY_CLASS(wxHtmlListmarkCell);
};

wxHtmlListmarkCell::wxHtmlListmarkCell(wxDC* dc, const wxColour& clr) : wxHtmlCell(), m_Brush(clr, wxBRUSHSTYLE_SOLID)
{
    m_Width =  dc->GetCharHeight();
    m_Height = dc->GetCharHeight();
    // bottom of list mark is lined with bottom of letters in next cell
    m_Descent = m_Height / 3;
}



void wxHtmlListmarkCell::Draw(wxDC& dc, int x, int y,
                              int WXUNUSED(view_y1), int WXUNUSED(view_y2),
                              wxHtmlRenderingInfo& WXUNUSED(info))
{
    dc.SetBrush(m_Brush);
    dc.DrawEllipse(x + m_PosX + m_Width / 3, y + m_PosY + m_Height / 3,
                   (m_Width / 3), (m_Width / 3));
}

//-----------------------------------------------------------------------------
// wxHtmlListCell
//-----------------------------------------------------------------------------

struct wxHtmlListItemStruct
{
    wxHtmlContainerCell *mark;
    wxHtmlContainerCell *cont;
    int minWidth;
    int maxWidth;
};

class wxHtmlListCell : public wxHtmlContainerCell
{
    private:
        wxBrush m_Brush;

        int m_NumRows;
        wxHtmlListItemStruct *m_RowInfo;
        void ReallocRows(int rows);
        void ComputeMinMaxWidths();
        int ComputeMaxBase(wxHtmlCell *cell);
        int m_ListmarkWidth;

    public:
        wxHtmlListCell(const wxHtmlTag& tag, wxHtmlContainerCell *parent);
        virtual ~wxHtmlListCell();
        void AddRow(wxHtmlContainerCell *mark, wxHtmlContainerCell *cont);
        virtual void Layout(int w) override;

    wxDECLARE_NO_COPY_CLASS(wxHtmlListCell);
};

wxHtmlListCell::wxHtmlListCell(const wxHtmlTag& tag, wxHtmlContainerCell *parent)
    : wxHtmlContainerCell(tag, parent)
{
    m_NumRows = 0;
    m_RowInfo = nullptr;
    m_ListmarkWidth = 0;
}

wxHtmlListCell::~wxHtmlListCell()
{
    if (m_RowInfo) free(m_RowInfo);
}

int wxHtmlListCell::ComputeMaxBase(wxHtmlCell *cell)
{
    if(!cell)
        return 0;

    wxHtmlCell *child = cell->GetFirstChild();

    while(child)
    {
        int base = ComputeMaxBase( child );
        if ( base > 0 ) return base + child->GetPosY();
        child = child->GetNext();
    }

    return cell->GetHeight() - cell->GetDescent();
}

void wxHtmlListCell::Layout(int w)
{
    wxHtmlCell::Layout(w);

    ComputeMinMaxWidths();
    m_Width = wxMax(m_Width, wxMin(w, GetMaxTotalWidth()));

    int s_width = m_Width - m_IndentLeft;

    int vpos = 0;
    for (int r = 0; r < m_NumRows; r++)
    {
        // do layout first time to layout contents and adjust pos
        m_RowInfo[r].mark->Layout(m_ListmarkWidth);
        m_RowInfo[r].cont->Layout(s_width - m_ListmarkWidth);

        const int base_mark = ComputeMaxBase( m_RowInfo[r].mark );
        const int base_cont = ComputeMaxBase( m_RowInfo[r].cont );
        const int adjust_mark = vpos + wxMax(base_cont-base_mark,0);
        const int adjust_cont = vpos + wxMax(base_mark-base_cont,0);

        m_RowInfo[r].mark->SetPos(m_IndentLeft, adjust_mark);
        m_RowInfo[r].cont->SetPos(m_IndentLeft + m_ListmarkWidth, adjust_cont);

        vpos = wxMax(adjust_mark + m_RowInfo[r].mark->GetHeight(),
                     adjust_cont + m_RowInfo[r].cont->GetHeight());
    }
    m_Height = vpos;
}

void wxHtmlListCell::AddRow(wxHtmlContainerCell *mark, wxHtmlContainerCell *cont)
{
    ReallocRows(++m_NumRows);
    m_RowInfo[m_NumRows - 1].mark = mark;
    m_RowInfo[m_NumRows - 1].cont = cont;
}

void wxHtmlListCell::ReallocRows(int rows)
{
    m_RowInfo = (wxHtmlListItemStruct*) realloc(m_RowInfo, sizeof(wxHtmlListItemStruct) * rows);
    m_RowInfo[rows - 1].mark = nullptr;
    m_RowInfo[rows - 1].cont = nullptr;
    m_RowInfo[rows - 1].minWidth = 0;
    m_RowInfo[rows - 1].maxWidth = 0;

    m_NumRows = rows;
}

void wxHtmlListCell::ComputeMinMaxWidths()
{
    if (m_NumRows == 0) return;

    m_MaxTotalWidth = 0;
    m_Width = 0;

    for (int r = 0; r < m_NumRows; r++)
    {
        wxHtmlListItemStruct& row = m_RowInfo[r];
        row.mark->Layout(1);
        row.cont->Layout(1);
        int maxWidth = row.cont->GetMaxTotalWidth();
        int width = row.cont->GetWidth();
        if (row.mark->GetWidth() > m_ListmarkWidth)
            m_ListmarkWidth = row.mark->GetWidth();
        if (maxWidth > m_MaxTotalWidth)
            m_MaxTotalWidth = maxWidth;
        if (width > m_Width)
            m_Width = width;
    }
    m_Width += m_ListmarkWidth + m_IndentLeft;
    m_MaxTotalWidth += m_ListmarkWidth + m_IndentLeft;
}

//-----------------------------------------------------------------------------
// wxHtmlListcontentCell
//-----------------------------------------------------------------------------

class wxHtmlListcontentCell : public wxHtmlContainerCell
{
public:
    wxHtmlListcontentCell(const wxHtmlTag& tag, wxHtmlContainerCell *p)
        : wxHtmlContainerCell(tag, p) {}

    virtual void Layout(int w) override {
        // Reset top indentation, fixes <li><p>
        SetIndent(0, wxHTML_INDENT_TOP);
        wxHtmlContainerCell::Layout(w);
    }
};

//-----------------------------------------------------------------------------
// The list handler:
//-----------------------------------------------------------------------------


TAG_HANDLER_BEGIN(OLULLI, "OL,UL,LI")

    TAG_HANDLER_VARS
        wxHtmlListCell *m_List;
        int m_Numbering;
                // this is number of actual item of list or 0 for dots

    TAG_HANDLER_CONSTR(OLULLI)
    {
        m_List = nullptr;
        m_Numbering = 0;
    }

    TAG_HANDLER_PROC(tag)
    {
        wxHtmlContainerCell *c;

        // List Item:
        if (m_List && tag.GetName() == wxT("LI"))
        {
            c = m_WParser->SetContainer(new wxHtmlContainerCell(m_List));
            c->SetAlignVer(wxHTML_ALIGN_TOP);

            wxHtmlContainerCell *mark = c;
            c->SetWidthFloat(2 * m_WParser->GetCharWidth(), wxHTML_UNITS_PIXELS);
            if (m_Numbering == 0)
            {
                // Centering gives more space after the bullet
                c->SetAlignHor(wxHTML_ALIGN_CENTER);
                c->InsertCell(new wxHtmlListmarkCell(m_WParser->GetDC(), m_WParser->GetActualColor()));
            }
            else
            {
                c->SetAlignHor(wxHTML_ALIGN_RIGHT);
                wxString markStr;
                markStr.Printf(wxT("%i. "), m_Numbering);
                c->InsertCell(new wxHtmlWordCell(markStr, *(m_WParser->GetDC())));
            }
            m_WParser->CloseContainer();

            c = m_WParser->OpenContainer();

            m_List->AddRow(mark, c);
            c = m_WParser->OpenContainer();
            m_WParser->SetContainer(new wxHtmlListcontentCell(tag, c));

            if (m_Numbering != 0) m_Numbering++;
        }

        // Begin of List (not-numbered): "UL", "OL"
        else if (tag.GetName() == wxT("UL") || tag.GetName() == wxT("OL"))
        {
            int oldnum = m_Numbering;

            if (tag.GetName() == wxT("UL")) m_Numbering = 0;
            else m_Numbering = 1;

            wxHtmlContainerCell *oldcont;
            oldcont = c = m_WParser->OpenContainer();

            wxHtmlListCell *oldList = m_List;
            m_List = new wxHtmlListCell(tag, c);
            m_List->SetIndent(2 * m_WParser->GetCharWidth(), wxHTML_INDENT_LEFT);

            ParseInner(tag);

            m_WParser->SetContainer(oldcont);
            m_WParser->CloseContainer();

            m_Numbering = oldnum;
            m_List = oldList;
            return true;
        }
        return false;

    }

TAG_HANDLER_END(OLULLI)


TAGS_MODULE_BEGIN(List)

    TAGS_MODULE_ADD(OLULLI)

TAGS_MODULE_END(List)

#endif
