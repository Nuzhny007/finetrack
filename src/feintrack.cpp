#include "feintrack.h"

////////////////////////////////////////////////////////////////////////////
namespace vl_feintrack
{
////////////////////////////////////////////////////////////////////////////

const float_t CFeinTrack::density_threshold = 0.1;
////////////////////////////////////////////////////////////////////////////

CFeinTrack::CFeinTrack()
    :
      use_square_segmentation(true),
      left_object_time1_sec(15),
      left_object_time2_sec(1 * 60),
      left_object_time3_sec(2 * 60),
      frame_width(0),
      frame_height(0),
      curr_color_type(buf_rgb24),
      fps(25),
      selection_time(25 / 2),
      curr_frame(0),
      cut_shadows(false),
      use_cuda(false),
      cuda_device_ind(0),
      pixel_size(3),
      use_morphology(true),
      min_region_width(5),
      min_region_height(5),
      left_padding(0),
      top_padding(0),
      need_background_update(true),
      del_objects_count(0),
      weight_threshold(0.1),
      weight_alpha(0.5 / 25),
      objects_count(0),
      left_objects_count(0),
      show_left_objects(true),
      show_objects(false),
      show_trajectory(false),
      use_recognition(false),
      bgrnd_type(norm_back),
      back_substractor(nullptr)
{
    set_bgrnd_type(bgrnd_type);

    selection_time = fps / 2;
    set_fps(fps);

    analyze_area = RECT_(0, 100, 0, 100);

    obj_rects.reserve(1500);
    del_objects.reserve(50);

    back_substractor->set_use_cuda(use_cuda);
}
////////////////////////////////////////////////////////////////////////////

CFeinTrack::~CFeinTrack()
{
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_bgrnd_type(bgrnd_substr_types bgrnd_type_)
{
    int sens_level = 60;
    bool detect_patches_of_sunlight = false;

    if (bgrnd_type != bgrnd_type_)
    {
        if (back_substractor)
        {
            sens_level = back_substractor->get_sensitivity();
            detect_patches_of_sunlight = back_substractor->get_detect_patches_of_sunlight();
        }
    }

    bgrnd_type = bgrnd_type_;

    switch (bgrnd_type)
    {
    case norm_back:
        back_substractor = std::unique_ptr<CBackSubstraction>(new CNormBackSubstraction());
        break;

    case gaussian_mixture:
        back_substractor = std::unique_ptr<CBackSubstraction>(new CGaussianMixtureBackSubstr());
        break;
    }

    back_substractor->set_use_cuda(use_cuda);
    back_substractor->set_fps(fps);
    back_substractor->set_sensitivity(sens_level);
    back_substractor->set_detect_patches_of_sunlight(detect_patches_of_sunlight);
    back_substractor->enable_back_update(need_background_update);
}
////////////////////////////////////////////////////////////////////////////

bgrnd_substr_types CFeinTrack::get_bgrnd_type() const
{
    return bgrnd_type;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_use_cuda(bool use_cuda_, int cuda_device_ind_)
{
    cuda_device_ind = cuda_device_ind_;
    if (use_cuda_)
    {
#ifdef USE_CUDA
        // ��������� ����� ���������, ������������ �������������� CUDA
        int count = 0;
        bool device_was_founded = false;
        if ((cudaGetDeviceCount(&count) == cudaSuccess) && count)
        {
            // ������ �� ���� ����������� � ������ �����������
            cudaDeviceProp prop;
            int cuda_device_counter = -1;     // ������� ����� ���������, �������������� CUDA
            int correct_cuda_device_ind = -1; // ������ ������ ����������, ��������������� CUDA
            for (int i = 0; i < count; ++i)
            {
                // �������� ������ ����������. ������ �������������� ��� ������
                // 9999 - ����� ����������, ���� ����� �������� ������� ����
                if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess &&
                        prop.major != 9999 &&
                        prop.minor != 9999)
                {
                    correct_cuda_device_ind = i;
                    if (cuda_device_counter + 1 == cuda_device_ind)
                    {
                        cudaSetDevice(i);
                        use_cuda = true;
                        if (back_substractor)
                            back_substractor->set_use_cuda(use_cuda);
                        if (show_objects)
                        {
                            set_show_objects(false);
                            set_show_objects(true);
                        }
                        device_was_founded = true;
                        ++cuda_device_counter;
                        break;
                    }
                    ++cuda_device_counter;
                }
            }
            // ��������� ���������� �� ������� - ��������� ������ ����������
            if (!device_was_founded && correct_cuda_device_ind != -1)
            {
                cudaSetDevice(correct_cuda_device_ind);
                use_cuda = true;
                if (back_substractor)
                    back_substractor->set_use_cuda(use_cuda);
                if (show_objects)
                {
                    set_show_objects(false);
                    set_show_objects(true);
                }
                device_was_founded = true;
            }
        }
        // ���������� ���������� �� �������
        if (!device_was_founded)
        {
            use_cuda = false;
            if (back_substractor)
                back_substractor->set_use_cuda(use_cuda);
        }
#else
        assert(false);
#endif
    }
    else
    {
        use_cuda = false;
        if (back_substractor)
            back_substractor->set_use_cuda(use_cuda);
        if (show_objects)
        {
            set_show_objects(false);
            set_show_objects(true);
        }
    }
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_use_cuda() const
{
    return use_cuda;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_cuda_device() const
{
    return cuda_device_ind;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_use_morphology() const
{
    return use_morphology;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_use_morphology(bool use_morphology_)
{
    use_morphology = use_morphology_;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_show_left_objects(bool show_left_objects_)
{
    show_left_objects = show_left_objects_;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_show_left_objects() const
{
    return show_left_objects;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_show_trajectory(bool show_trajectory_)
{
    show_trajectory = show_trajectory_;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_show_trajectory() const
{
    return show_trajectory;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_min_region_width(int min_region_width_)
{
    min_region_width = min_region_width_;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_min_region_width() const
{
    return min_region_width;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_selection_time(int selection_time_)
{
    selection_time = selection_time_;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_selection_time() const
{
    return selection_time;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_min_region_height(int min_region_height_)
{
    min_region_height = min_region_height_;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_min_region_height() const
{
    return min_region_height;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_detect_patches_of_sunlight() const
{
    if (back_substractor)
        return back_substractor->get_detect_patches_of_sunlight();
    else
        return true;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_detect_patches_of_sunlight(bool detect_patches_of_sunlight_)
{
    if (back_substractor)
        back_substractor->set_detect_patches_of_sunlight(detect_patches_of_sunlight_);
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_cut_shadows() const
{
    return cut_shadows;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_cut_shadows(bool cut_shadows_)
{
    cut_shadows = cut_shadows_;
}
////////////////////////////////////////////////////////////////////////////

RECT_ CFeinTrack::get_analyze_area() const
{
    return analyze_area;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_analyze_area(const RECT_ &analyze_area_)
{
    if (analyze_area == analyze_area_)
        return;

    analyze_area = analyze_area_;
    if (show_objects)
    {
        set_show_objects(false);
        set_show_objects(true);
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_sensitivity(int sens_level)
{
    if (back_substractor)
        back_substractor->set_sensitivity(sens_level);
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_sensitivity() const
{
    if (back_substractor)
        return back_substractor->get_sensitivity();
    else
        return 60;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_left_object_time1_sec() const
{
    return left_object_time1_sec;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_left_object_time1_sec(int left_object_time1_sec_)
{
    left_object_time1_sec = left_object_time1_sec_;
    left_object_time1 = left_object_time1_sec * fps;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_left_object_time2_sec() const
{
    return left_object_time2_sec;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_left_object_time2_sec(int left_object_time2_sec_)
{
    left_object_time2_sec = left_object_time2_sec_;
    left_object_time2 = left_object_time2_sec * fps;
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_left_object_time3_sec() const
{
    return left_object_time3_sec;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_left_object_time3_sec(int left_object_time3_sec_)
{
    left_object_time3_sec = left_object_time3_sec_;
    left_object_time3 = left_object_time3_sec * fps;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::enable_back_update(bool enable_val)
{
    need_background_update = enable_val;
    if (back_substractor)
        back_substractor->enable_back_update(enable_val);
}
////////////////////////////////////////////////////////////////////////////

int CFeinTrack::get_fps() const
{
    return fps;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_fps(int new_fps)
{
    fps = new_fps;
    weight_alpha = 0.5 / fps;

    left_object_time0 = 1 * fps;
    left_object_time1 = left_object_time1_sec * fps;
    left_object_time2 = left_object_time2_sec * fps;
    left_object_time3 = left_object_time3_sec * fps;

    if (back_substractor)
        back_substractor->set_fps(fps);
}
////////////////////////////////////////////////////////////////////////////

#if !ADV_OUT
int CFeinTrack::new_frame(const uchar* buf, uint32_t pitch, uint32_t width, uint32_t height, color_type buf_type)
#else
int CFeinTrack::new_frame(const uchar* buf, uint32_t pitch, uint32_t width, uint32_t height, color_type buf_type, uchar* adv_buf_rgb24)
#endif
{
    // ���� ������� ���������� �� �����, �� ������ �� ��������
    if (!show_objects)
        return 0;

    pixel_size = get_pixel_size<int>(buf_type);

    // ��������� �������� �����, ��������� � �������� ������� �������
    if (!use_cuda)
    {
        left_padding = (analyze_area.left * width) / 100;
        top_padding = (analyze_area.top * height) / 100;
        recalc_correct_zones();
        recalc_correct_lines();

        buf += top_padding * pitch + pixel_size * left_padding;
#if ADV_OUT
        adv_buf_rgb24 += top_padding * 3 * width + 3 * left_padding;
#endif
        width = ((analyze_area.right - analyze_area.left) * width) / 100;
        height = ((analyze_area.bottom - analyze_area.top) * height) / 100;
    }
    else
    {
        left_padding = 0;
        top_padding = 0;
    }

    // �������� �� ����� ����������, ���� �� ������������� � �.�.
    bool tmp_use_cuda = use_cuda;
    if (back_substractor->init(width, height, buf_type, use_cuda))
    {
        frame_width = width;
        frame_height = height;
        curr_color_type = buf_type;

        segmentator.init(width, height, use_cuda);
        if (tmp_use_cuda != use_cuda)
            back_substractor->init(width, height, buf_type, use_cuda);

        curr_frame = 0;

        objects_count = 0;
        del_objects_count = 0;
        left_objects_count = 0;

        shady_left_objects.clear();

        lefted_objects.clear();

        objects_history.clear();

        // ������� ������� ����� ��� ����� ����������
        return 0;
    }

    // ��������� ����
#if !ADV_OUT
#ifdef USE_CUDA
    if (!back_substractor->background_substraction(curr_frame, buf, pitch, segmentator.get_mask(), segmentator.get_device_mask()))
#else
    if (!back_substractor->background_substraction(curr_frame, buf, pitch, segmentator.get_mask()))
#endif
#else
#ifdef USE_CUDA
    if (!back_substractor->background_substraction(curr_frame, buf, pitch, segmentator.get_mask(), segmentator.get_device_mask(), adv_buf_rgb24))
#else
    if (!back_substractor->background_substraction(curr_frame, buf, pitch, segmentator.get_mask(), adv_buf_rgb24))
#endif

#endif
    {
        return 0;
    }

    // �������� �������������� ���������� "��������" ��� ���������� ���� ���� ���������� ��������
    if (use_morphology)
    {
        segmentator.morphology_open(use_cuda);
    }

#if ADV_OUT
    //segmentator.draw_mask(use_cuda, adv_buf_rgb24);
#endif

    // ���� ������������ cuda � ��������� �������������, �� �������� ����� �� ����������� � ���������
    if (use_recognition && use_cuda)
    {
#ifdef USE_CUDA
        segmentator.copy_gpu2cpu();
#else
        assert(false);
#endif
    }

    // C���������� �������� ��������� �����
    regions.clear();
    if (use_cuda)
    {
#ifdef USE_CUDA
        segmentator.cuda_segmentation(regions);
#else
        assert(false);
#endif
    }
    else
    {
        if (use_square_segmentation)
        {
            segmentator.square_segmentation(regions);
        }
        else
        {
            segmentator.iterative_segmentation(regions);
        }
    }

    // ��������������� ������ � ��������� ��������
    regions_preprocessing(buf, pitch);

    // �������� � ������ ��������� �������� �� ��������� ��������� �� ������� ����� ��������
#if !ADV_OUT
    tracking_objects(buf, pitch);
#else
    tracking_objects(buf, pitch, adv_buf_rgb24);
#endif

    // ������ ����������� ���������
    analyze_lefted_objects();

    if (++curr_frame == fps)
    {
        curr_frame = 0;
    }

    return 1;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::add_uid_to_del_objects(unsigned int uid)
{
    // ���������� � ������ �������� ��������
    if (++del_objects_count > del_objects.size())
        del_objects.push_back(uid);
    else
        del_objects[del_objects_count - 1] = uid;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::del_object(std::unique_ptr<CTrackingObject>& object, bool del_adv_data)
{
    if (del_adv_data)
    {
        // �������� �� ������ �������� ����������� ���������
        del_from_shady_left_objects(object->uid);
    }
    add_uid_to_del_objects(object->uid);
    object = nullptr;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::regions_preprocessing(const uchar* buf, uint32_t pitch)
{
    // ����� ��������� ������� � ������� � ��������� ���������� ����� �������
    bool need_update = (curr_frame % fps == fps - 1);
    for (regions_container::iterator it_r = regions.begin(); it_r != regions.end();)
    {
        int min_width = min_region_width;
        int min_height = min_region_height;
        const CZone* zone = get_zone(*it_r);
        if (zone)
        {
            min_width = zone->min_obj_width;
            min_height = zone->min_obj_height;
        }

        if ((it_r->width() < min_width) ||
                (it_r->height() < min_height) ||
                (it_r->density() < density_threshold))
        {
            if (need_update)
            {
                back_substractor->update_statistic_in_region(buf, pitch, *it_r);
            }

            it_r = regions.erase(it_r);
        }
        else
        {
            // ��������� ���� � ������ �������������
            if (cut_shadows)
            {
                cut_shadow(*it_r);
            }
            ++it_r;
        }
    }
}
////////////////////////////////////////////////////////////////////////////

#if !ADV_OUT
void CFeinTrack::tracking_objects(const uchar* buf, uint32_t pitch)
#else
void CFeinTrack::tracking_objects(const uchar* buf, uint32_t pitch, uchar* adv_buf_rgb24)
#endif
{
    // �������� ���������� ��������� ��������
    objects_count = 0;
    // �������� ���������� �������� ��������
    del_objects_count = 0;

    // ��� ��������, ��������� �� ���������� ������, ���� ���������� �������
    for (objects_container::iterator iter_obj = objects_history.begin(); iter_obj != objects_history.end();)
    {
#if 1
#if ADV_OUT
        RECT_ r((*iter_obj)->get_rect());
        paint_h_line<0, 0xff, 0, 3>(adv_buf_rgb24, 3 * frame_width, r.left, r.right, r.top);
        paint_h_line<0, 0xff, 0, 3>(adv_buf_rgb24, 3 * frame_width, r.left, r.right + 1, r.bottom);

        paint_v_line<0, 0xff, 0, 3>(adv_buf_rgb24, 3 * frame_width, r.left, r.top, r.bottom);
        paint_v_line<0, 0xff, 0, 3>(adv_buf_rgb24, 3 * frame_width, r.right, r.top, r.bottom);
#endif
#endif

        // ���� ���������� ��� ������� ������ � ������ ���������� ���������� ��������� �������
#if 1
        regions_container::iterator find_region(find_region_by_center((*iter_obj)->get_new_center_x(), (*iter_obj)->get_new_center_y(), (*iter_obj)->width(), (*iter_obj)->height()));
#else
        regions_container::iterator find_region(find_region_by_hist(buf, pitch, *iter_obj));
#endif

        // ���� ������ �� ������,
        if (find_region == regions.end() &&
                (*iter_obj)->get_new_center_x() != (*iter_obj)->get_last_center_x() &&
                (*iter_obj)->get_new_center_y() != (*iter_obj)->get_last_center_y())
        {
            // �� ��������� ����� � ������������ ������, ����������� �� ���������� ����
#if 1
            find_region = find_region_by_center((*iter_obj)->get_last_center_x(), (*iter_obj)->get_last_center_y(), (*iter_obj)->width(), (*iter_obj)->height());
#else
            regions_container::iterator find_region(find_region_by_hist(buf, pitch, *iter_obj));
#endif
        }
        // ����, �������, ���������� ������ �������, ��:
        if (find_region != regions.end())
        {

            bool is_merge(false);
#if 0
            //� ������� ������� �� ���� ����������� � �������
            if (!(*iter_obj)->have_merge_object())
            {
                //���� ������, � ������� �������� �����������
                objects_container::iterator merge_obj = get_object_by_region(*find_region, iter_obj);
                if (merge_obj != objects_history.end())
                {
                    if (!(*merge_obj)->have_merge_object())
                    {
                        //���� ������ ������ ������� ������ �������� ������������ �� ���� ��������, �� ���������� �������
                        if ((find_region->width() > (*iter_obj)->width()) &&
                                (find_region->height() > (*iter_obj)->height()))
                        {
                            if ((find_region->width() > (*merge_obj)->width()) &&
                                    (find_region->height() > (*merge_obj)->height()))
                            {
                                //���������� �������
                                (*iter_obj)->add_merge_obj(**merge_obj);
                                //������� ������, � ������� ���� ����������� ������� (��� ���������, �.�. ������������ list)
                                del_object(*merge_obj, false);
                                objects_history.erase(merge_obj);

                                is_merge = true;
                            }
                        }
                    }
                }
            }
            else //������ ������ �������� � ���� 2 ������� �������� �������
            {
                //���� �������, ������� ����� �� ������������ �� ��� �������
                CTrackingObject *merge_object1 = (*iter_obj)->get_merge_object(1);
                CTrackingObject *merge_object2 = (*iter_obj)->get_merge_object(2);
                regions_container::iterator region1(find_region_by_center(merge_object1->get_new_center_x(), merge_object1->get_new_center_y(), merge_object1->width(), merge_object1->height()));
                if (region1 != regions.end())
                {
                    regions_container::iterator region2(find_region_by_center(merge_object2->get_new_center_x(), merge_object2->get_new_center_y(), merge_object2->width(), merge_object2->height()));
                    if (region2 != regions.end() &&
                            region1 != region2)
                    {
                        //���������� ������� ������� � �� ������� ����������
                        //��������� ��� ������� � ������, � ������������ �������

                        //1. ��������� ����� ��� �������
                        merge_object1->weight = (1 - weight_alpha) * merge_object1->weight + weight_alpha;
                        //2. ����������� ����� ����� �������
                        merge_object1->life_time++;
                        //3. ���������� ���������� �������
                        merge_object1->set_rect(region1->get_left(), region1->get_right(), region1->get_top(), region1->get_bottom());
                        //4. �������� ���������� ������, �� ������� ������ �� ��� ������
                        merge_object1->frames_left = 0;
                        //����� ����� �������� ������ �������
                        merge_object1->set_last_center(region1->get_center_x(), region1->get_center_y());

                        //1. ��������� ����� ��� �������
                        merge_object2->weight = (1 - weight_alpha) * merge_object2->weight + weight_alpha;
                        //2. ����������� ����� ����� �������
                        merge_object2->life_time++;
                        //3. ���������� ���������� �������
                        merge_object2->set_rect(region2->get_left(), region2->get_right(), region2->get_top(), region2->get_bottom());
                        //4. �������� ���������� ������, �� ������� ������ �� ��� ������
                        merge_object2->frames_left = 0;
                        //����� ����� �������� ������ �������
                        merge_object2->set_last_center(region2->get_center_x(), region2->get_center_y());

                        //���������� ������� �� �����
                        mstring zone_name;
                        if (is_in_zone(*region1, &zone_name))
                            add_object_to_out_rects(*region1, *merge_object1, CObjRect::unknown, zone_name);
                        if (is_in_zone(*region2, &zone_name))
                            add_object_to_out_rects(*region2, *merge_object2, CObjRect::unknown, zone_name);

                        //������� ��������� � ����� ������
                        objects_history.push_front(merge_object2);
                        objects_history.push_front(merge_object1);

                        //"������������" ������ ���������
                        (*iter_obj)->set_merge_objects_to_null();
                        del_object(*iter_obj, false);
                        iter_obj = objects_history.erase(iter_obj);

                        //� ����� ��������� �������, ��������������� ���������� ��������
                        regions.erase(region1);
                        regions.erase(region2);
                        continue;
                    }
                }
                //���������� �� ��������� - ����������� �����, � ������� �������� ������� ���������� "���������"
                (*iter_obj)->inc_merge_frames();
            }
#endif

            // �������� ��������� �������:
            // 1. ��������� ����� ��� �������
            (*iter_obj)->weight = (1 - weight_alpha) * (*iter_obj)->weight + weight_alpha;
            // 2. ����������� ����� ����� �������
            (*iter_obj)->life_time++;
            // 3. ���������� ���������� �������
            (*iter_obj)->set_rect(find_region->get_left(), find_region->get_right(), find_region->get_top(), find_region->get_bottom());
            // 4. �������� ���������� ������, �� ������� ������ �� ��� ������
            (*iter_obj)->frames_left = 0;

            // ������ ���������, ���� �� ���������� ���������� �����
            if ((*iter_obj)->life_time > selection_time)
            {
                // ������ ��������� ������������� � ������� �������� ���������� ������������ ����������
                if ((*iter_obj)->get_left_frames() == left_object_time0)
                {
                    add_to_shady_left_objects(**iter_obj);
                }

                // ���� ������ ���������
                if (((*iter_obj)->get_left_frames() < left_object_time1) || is_merge)
                {
                    // �������� �� ������ � ���� ��������
                    mstring zone_name;
                    if (is_in_zone(*find_region, &zone_name))
                    {
                        if (!is_merge)
                        {
                            // ��������� �� ����������� � ������� � ��������� ���������� ������ ������� � �������� ��� ��������
                            with_line_intersect(**iter_obj, find_region->get_center_x(), find_region->get_center_y());

                            // ����� ����� �������� ������ �������
                            (*iter_obj)->set_last_center(find_region->get_center_x(), find_region->get_center_y());
                            // ������� ������������� �� ������ ��������, �������� ���������� ������������
                            if (!(*iter_obj)->get_left_frames())
                            {
                                del_uid_from_shady_left_objects((*iter_obj)->uid);
                            }
                            else
                            {
                                inc_time_shady_left_objects((*iter_obj)->uid);
                            }
                        }

                        object_types type_now = unknown_object;
                        if (use_recognition)
                        {
                            // �������������
                            type_now = recognizer.recognize_object(*find_region, buf, pitch, frame_width, frame_height, &segmentator.get_mask()[0]);
                            (*iter_obj)->set_new_type(type_now);
                        }

                        // ���������� ������ �� �����
                        add_object_to_out_rects(*find_region, **iter_obj, type_now, zone_name);
                    }
                    else // ������ �� ����� � ���� ��������
                    {
                        if (!is_merge)
                        {
                            // ����� ����� �������� ������ �������
                            (*iter_obj)->set_last_center(find_region->get_center_x(), find_region->get_center_y());
                            // ������� ������������� �� ������ ��������, �������� ���������� ������������
                            if (!(*iter_obj)->get_left_frames())
                            {
                                del_uid_from_shady_left_objects((*iter_obj)->uid);
                            }
                            else
                            {
                                inc_time_shady_left_objects((*iter_obj)->uid);
                            }
                        }
                    }
                }
                else // ������ �� ��������� - ���������� ����������� ���������
                {
                    // �������� �� ����������� ������� � ���� ��������
                    if (is_in_zone(*find_region, nullptr))
                    {
                        // C����� ����������� �������
                        lefted_objects.push_back(CLeftObjView(left_object_time3 - left_object_time1, find_region->get_left(), find_region->get_right(), find_region->get_top(), find_region->get_bottom()));
                    }

                    if (need_background_update)
                    {
                        // ��������� ������, ����� ��������������� ��������� ��������
                        find_region->resize_to_max_granz(frame_width - 1, frame_height - 1);
                        // � �������� ���������� ������ ����
                        back_substractor->reset_statistic_in_region(buf, pitch, *find_region);
                    }

                    // ������� ��������� ������, ����� �������� ���������� � ������� ���������
                    regions.erase(find_region);

                    // ������� �� ������ ��������
                    del_object(*iter_obj, true);
                    iter_obj = objects_history.erase(iter_obj);
                    continue;
                }
            }
            else // ������ ���������� ������������ �����
            {
                if (!is_merge)
                {
                    // ����� ����� �������� ������ �������
                    (*iter_obj)->set_last_center(find_region->get_center_x(), find_region->get_center_y());
                    // ������� ������������� �� ������ ��������, �������� ���������� ������������
                    if (!(*iter_obj)->get_left_frames())
                    {
                        del_uid_from_shady_left_objects((*iter_obj)->uid);
                    }
                    else
                    {
                        inc_time_shady_left_objects((*iter_obj)->uid);
                    }
                }
            }

            // ������� ��������� ������, ����� �������� ���������� � ������� ���������
            regions.erase(find_region);
        }
        else // ���� ���������� ������ �� ������, ��:
        {
            // 1. ������������� ��� �������
            (*iter_obj)->weight = (1 - weight_alpha) * (*iter_obj)->weight;
            // 2. ���� ��� ������ �����������, �� ������ ���������
            if ((*iter_obj)->weight < weight_threshold)
            {
                del_object(*iter_obj, true);
                iter_obj = objects_history.erase(iter_obj);
                continue;
            }
            // 3. ������������� ��� ����� � ������������ � ��������� ��������� �������
            (*iter_obj)->recalc_center();
            // 4. ���� ������ ����� �� ������� ����� - �������
            if (!in_range<int>((*iter_obj)->get_last_center_x(), 0, frame_width) ||
                    !in_range<int>((*iter_obj)->get_last_center_y(), 0, frame_height))
            {
                del_object(*iter_obj, true);
                iter_obj = objects_history.erase(iter_obj);
                continue;
            }
            // 5. ����������� ����� ������, �� ������� ������ �� ��� ������
            (*iter_obj)->frames_left++;

#if 1           // ����������������, ��� ��� ��������� ����� ����� ����� ��������, ����� �������� ��� ������������
            //6. ���������� ������ �� �����
            if ((*iter_obj)->life_time > selection_time &&
                    (*iter_obj)->frames_left < selection_time / 4)
            {
                //�������� �� ����������� ������� � ���� ��������
                mstring zone_name;
                if (is_in_zone(**iter_obj, &zone_name))
                {
                    add_object_to_out_rects(**iter_obj, **iter_obj, (*iter_obj)->get_type(), zone_name);
                }
            }
#endif
        }

        ++iter_obj;
    }

    // ��� ���������� �������� (� ������� ��� ����������� �������) ������ ����� �������, �� ���� �� �������
    for (regions_container::iterator iter_reg = regions.begin(); iter_reg != regions.end(); ++iter_reg)
    {
        objects_history.push_back(std::unique_ptr<CTrackingObject>(new CTrackingObject(iter_reg->get_center_x(), iter_reg->get_center_y(), get_free_uid())));

        (*objects_history.rbegin())->set_rect(iter_reg->get_left(), iter_reg->get_right(), iter_reg->get_top(), iter_reg->get_bottom());
    }
    //O������ ������ ��������
    regions.clear();

    // ��������� ������� �� ������� �����
    objects_history.sort(CTrackingObject::life_bigger);

    // ������� �������, �� ���������������� � ������� ���������� �������
    analyze_shady_left_objects();
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::add_to_shady_left_objects(CTrackingObject &obj)
{
    CShadyLeftObj new_obj(obj.uid, obj.get_left_frames(), obj.get_left(), obj.get_right(), obj.get_top(), obj.get_bottom());
    // ���� ���������� ������ � ������� � ��������� �����������, �� ��� ������� � ����� ����������
    for (std::list<CShadyLeftObj>::iterator iter = shady_left_objects.begin(); iter != shady_left_objects.end();)
    {
        if ((abs(iter->rect.center_x() - obj.get_last_center_x()) < 2 * obj.get_left_epsilon()) &&
                (abs(iter->rect.center_y() - obj.get_last_center_y()) < 2 * obj.get_left_epsilon()))
        {
            new_obj.life_time += iter->life_time;
            iter = shady_left_objects.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    shady_left_objects.push_back(new_obj);
    obj.set_left_frames(new_obj.life_time);
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::del_from_shady_left_objects(unsigned int obj_uid)
{
    for (std::list<CShadyLeftObj>::iterator iter = shady_left_objects.begin(); iter != shady_left_objects.end(); ++iter)
    {
        if (iter->real_obj_uid == obj_uid)
        {
            iter = shady_left_objects.erase(iter);
            return;
        }
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::del_uid_from_shady_left_objects(unsigned int obj_uid)
{
    for (std::list<CShadyLeftObj>::iterator iter = shady_left_objects.begin(); iter != shady_left_objects.end(); ++iter)
    {
        if (iter->real_obj_uid == obj_uid)
        {
            iter->real_obj_uid = 0;
            return;
        }
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::inc_time_shady_left_objects(unsigned int obj_uid)
{
    for (std::list<CShadyLeftObj>::iterator iter = shady_left_objects.begin(); iter != shady_left_objects.end(); ++iter)
    {
        if (iter->real_obj_uid == obj_uid)
        {
            iter->life_time++;
            return;
        }
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::analyze_shady_left_objects()
{
    for (std::list<CShadyLeftObj>::iterator iter = shady_left_objects.begin(); iter != shady_left_objects.end();)
    {
        if (!iter->real_obj_uid)
        {
            iter->not_detect_time++;
            if (iter->not_detect_time > left_object_time1)
                iter = shady_left_objects.erase(iter);
            else
                ++iter;
        }
        else
        {
            ++iter;
        }
    }
}
////////////////////////////////////////////////////////////////////////////

objects_container::iterator CFeinTrack::get_object_by_region(const CObjectRegion& region, objects_container::iterator from_obj)
{
    objects_container::iterator find_object(objects_history.end());
    int min_x(std::numeric_limits<int>::max() / 2 - 1);
    int min_y(std::numeric_limits<int>::max() / 2 - 1);

    int tmp_x, tmp_y;
    int c_x = region.get_center_x(), c_y = region.get_center_y();

    int motion_delta_x = 2 * (*from_obj)->width();
    int motion_delta_y = 2 * (*from_obj)->height();

    // ���� ������, ����� �������� �������� ��������� � ������ �������
    for (objects_container::iterator iter = ++from_obj; iter != objects_history.end(); ++iter)
    {
        tmp_x = abs((*iter)->get_new_center_x() - c_x);
        tmp_y = abs((*iter)->get_new_center_y() - c_y);

        if ((tmp_x + tmp_y < min_x + min_y) && (tmp_x < motion_delta_x) && (tmp_y < motion_delta_y))
        {
            min_x = tmp_x;
            min_y = tmp_y;
            find_object = iter;
        }
    }
    return find_object;
}
////////////////////////////////////////////////////////////////////////////

regions_container::iterator CFeinTrack::find_region_by_hist(const uchar* buf, int pitch, const std::unique_ptr<CTrackingObject>& obj)
{
    int c_x = obj->get_new_center_x();
    int c_y = obj->get_new_center_y();

    int left = obj->get_left();
    int top = obj->get_top();
    int width = obj->width();
    int height = obj->height();

    regions_container::iterator find_region(regions.end());

    if ((c_x < 0) || (c_y < 0))
    {
        return find_region;
    }

    int min_x(std::numeric_limits<int>::max() / 2 - 1);
    int min_y(std::numeric_limits<int>::max() / 2 - 1);

    hist_cont standart_hist(256, 0);
    // ����������� ����������� ������������ �������;
    calculate_hist(buf, pitch, pixel_size, left, top, width, height, standart_hist, frame_width, &segmentator.get_mask()[0]);

    // ��� �� ������ ��������� ������
    bool isFound = false;

    width *= 2;
    height *= 2;

    // ���� ������, ����� �������� �������� ��������� � ������ ���������������� �������
    for (regions_container::iterator iter_reg = regions.begin(); iter_reg != regions.end(); ++iter_reg)
    {
        int tmp_x = abs(iter_reg->get_center_x() - c_x);
        int tmp_y = abs(iter_reg->get_center_y() - c_y);

        if ((tmp_x + tmp_y < min_x + min_y) && (tmp_x < width) && (tmp_y < height))
        {
            min_x = tmp_x;
            min_y = tmp_y;
            find_region = iter_reg;
            isFound = true;
        }
    }

#if 1
    if (isFound)
    {
        hist_cont suggestion_hist(256, 0);
        calculate_hist(buf, pitch, pixel_size, find_region->get_left(), find_region->get_top(),
                       find_region->width(), find_region->height(), suggestion_hist, frame_width, &segmentator.get_mask()[0]);
        // ���� ���������� ��������� ������� ������
        double distance = bhattacharrya_dist(standart_hist, suggestion_hist);
        if (distance < 0.6)
            find_region = regions.end();
        else
            isFound = isFound;
    }
#endif
    return find_region;
}
////////////////////////////////////////////////////////////////////////////

regions_container::iterator CFeinTrack::find_region_by_center(int c_x, int c_y, int width, int height/*, Hist& object_hist*/)
{
    regions_container::iterator find_region(regions.end());
    int min_x(std::numeric_limits<int>::max() / 2 - 1);
    int min_y(std::numeric_limits<int>::max() / 2 - 1);

    width *= 2;
    height *= 2;

#if 0
    RECT_ source_rect = RECT_(c_x, c_x + width, c_y, c_y + height);
    int maxIntersectionArea = 1;
#endif

    // ���� ������, ����� �������� �������� ��������� � ������ ���������������� �������
    for (regions_container::iterator iter_reg = regions.begin(); iter_reg != regions.end(); ++iter_reg)
    {
#if 0
        int intersectionArea = get_intersect_area<int>(source_rect,
                                                       RECT_(iter_reg->get_left(), iter_reg->get_right(), iter_reg->get_top(), iter_reg->get_bottom()));

        if (intersectionArea > maxIntersectionArea)
        {
            find_region = iter_reg;
            maxIntersectionArea = intersectionArea;
        }
#else
        int tmp_x = abs(iter_reg->get_center_x() - c_x);
        int tmp_y = abs(iter_reg->get_center_y() - c_y);

        if ((tmp_x + tmp_y < min_x + min_y) && (tmp_x < width) && (tmp_y < height))
        {
            min_x = tmp_x;
            min_y = tmp_y;
            find_region = iter_reg;
        }
#endif
    }
    return find_region;
}
////////////////////////////////////////////////////////////////////////////

unsigned int CFeinTrack::get_free_uid() const
{
    // �������� ������� ���������
    unsigned int ret_val = 1;
    for (; ret_val < std::numeric_limits<unsigned int>::max(); ++ret_val)
    {
        // ����� ����� �������� ��������
        if (std::find(del_objects.begin(), del_objects.begin() + del_objects_count, ret_val) != del_objects.begin() + del_objects_count)
            continue;

        // ����� ����� ������������
        bool find_uid = false;
        for (objects_container::const_iterator iter = objects_history.begin(); iter != objects_history.end(); ++iter)
        {
            if ((ret_val == (*iter)->uid) || (*iter)->has_merge_object(ret_val))
            {
                find_uid = true;
                break;
            }
        }
        if (!find_uid)
            break;
    }
    return ret_val;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::add_left_object_to_out_rects(const CLeftObjView &left_obj, CLeftObjRect::types type)
{
    // ����� ������� ������� �������� ������ ��������� ������: ������ left_obj_rects ������� �� �����������
    // �� ����� ������ ����������� ���� ������, ���� �� ����������� ������ ���������� �������� ������, ��� �� ����������
    if (++left_objects_count > left_obj_rects.size())
    {
        left_obj_rects.push_back(CLeftObjRect(left_obj.rect, type));
        if (left_padding)
        {
            left_obj_rects[left_objects_count - 1].left += left_padding;
            left_obj_rects[left_objects_count - 1].right += left_padding;
        }
        if (top_padding)
        {
            left_obj_rects[left_objects_count - 1].top += top_padding;
            left_obj_rects[left_objects_count - 1].bottom += top_padding;
        }
    }
    else
    {
        left_obj_rects[left_objects_count - 1].left = left_obj.rect.left + left_padding;
        left_obj_rects[left_objects_count - 1].right = left_obj.rect.right + left_padding;
        left_obj_rects[left_objects_count - 1].top = left_obj.rect.top + top_padding;
        left_obj_rects[left_objects_count - 1].bottom = left_obj.rect.bottom + top_padding;
        left_obj_rects[left_objects_count - 1].type = type;
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::analyze_lefted_objects()
{
    left_objects_count = 0;

    // ���� ������� ������������, �� ����� ������ �������
    lefted_objects.sort(CLeftObjView::bigger);
    for (std::list<CLeftObjView>::iterator iter1 = lefted_objects.begin(); iter1 != lefted_objects.end(); ++iter1)
    {
        std::list<CLeftObjView>::iterator iter2 = iter1;
        ++iter2;
        for (; iter2 != lefted_objects.end();)
        {
            if (segments_superposition(iter1->rect.left, iter1->rect.right, iter2->rect.left, iter2->rect.right) &&
                    segments_superposition(iter1->rect.top, iter1->rect.bottom, iter2->rect.top, iter2->rect.bottom))
            {
                iter2 = lefted_objects.erase(iter2);
            }
            else
            {
                ++iter2;
            }
        }
    }

    // ��������� ������ ��� �������
    for (std::list<CLeftObjView>::iterator iter = lefted_objects.begin(); iter != lefted_objects.end();)
    {
        // ������� ����������� ��������, ����� ������� ������ ������������ ������� (left_object_life_time)
        if (iter->life_time < 1)
        {
            iter = lefted_objects.erase(iter);
        }
        else
        {
            // ��������� ����� ����� �������
            iter->life_time--;

            // ��������� � ������ ����������� ��������
            add_left_object_to_out_rects(*iter, ((left_object_time3 - iter->life_time < left_object_time2)? CLeftObjRect::first: CLeftObjRect::second));
            ++iter;
        }
    }
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::with_line_intersect(const CTrackingObject &obj, int new_center_x, int new_center_y)
{
    bool ret_val(false);

    if (correct_lines.empty())
        return ret_val;

    const float_t r = 50000.0;
    float_t lx1, lx2, ly1, ly2;
    float_t objx1, objx2, objy1, objy2;

    for (size_t i = 0; i < correct_lines.size(); ++i)
    {
        // �������� ����������� ���������� ���� ���������� �������� ������� � ���������������� �����

        lx1 = wnd_to_x(correct_lines[i].x1, frame_width, -r, r);
        lx2 = wnd_to_x(correct_lines[i].x2, frame_width, -r, r);
        ly1 = wnd_to_x(correct_lines[i].y1, frame_height, -r, r);
        ly2 = wnd_to_x(correct_lines[i].y2, frame_height, -r, r);

        objx1 = wnd_to_x(obj.get_last_center_x(), frame_width, -r, r);
        objy1 = wnd_to_x(obj.get_last_center_y(), frame_height, -r, r);
        objx2 = wnd_to_x(new_center_x, frame_width, -r, r);
        objy2 = wnd_to_x(new_center_y, frame_height, -r, r);

        if (is_intersect(lx1, ly1, lx2, ly2, objx1, objy1, objx2, objy2))
        {
#if 0
            // ���������� ������ � ����� ������� ��������� �����������
            // ��� ����� ���������� �����, �������� ������� �� ������� 1
            float_t x(0), y(0);
            if ((correct_lines[i].x1 <= correct_lines[i].x2) && (correct_lines[i].y1 > correct_lines[i].y2))
            {
                x = wnd_to_x((correct_lines[i].x1 + correct_lines[i].x2) / 2 - 10, frame_width, -r, r);
                y = wnd_to_x((correct_lines[i].y1 + correct_lines[i].y2) / 2 - 10, frame_height, -r, r);
            }
            else
            {
                if ((correct_lines[i].x1 <= correct_lines[i].x2) && (correct_lines[i].y1 <= correct_lines[i].y2))
                {
                    x = wnd_to_x((correct_lines[i].x1 + correct_lines[i].x2) / 2 + 10, frame_width, -r, r);
                    y = wnd_to_x((correct_lines[i].y1 + correct_lines[i].y2) / 2 - 10, frame_height, -r, r);
                }
                else
                {
                    if ((correct_lines[i].x1 > correct_lines[i].x2) && (correct_lines[i].y1 > correct_lines[i].y2))
                    {
                        x = wnd_to_x((correct_lines[i].x1 + correct_lines[i].x2) / 2 - 10, frame_width, -r, r);
                        y = wnd_to_x((correct_lines[i].y1 + correct_lines[i].y2) / 2 + 10, frame_height, -r, r);
                    }
                    else
                    {
                        if ((correct_lines[i].x1 > correct_lines[i].x2) && (correct_lines[i].y1 <= correct_lines[i].y2))
                        {
                            x = wnd_to_x((correct_lines[i].x1 + correct_lines[i].x2) / 2 + 10, frame_width, -r, r);
                            y = wnd_to_x((correct_lines[i].y1 + correct_lines[i].y2) / 2 + 10, frame_height, -r, r);
                        }
                    }
                }
            }
            direction = is_intersect(lx1, ly1, lx2, ly2, objx1, objy1, x, y) ? 1 : 0;
#endif
            ret_val = true;
        }
    }
    return ret_val;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_lines_list(const lines_cont& lines_)
{
    lines = lines_;
    recalc_correct_lines();
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::get_lines_list(lines_cont& lines_) const
{
    lines_ = lines;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::recalc_correct_lines()
{
    correct_lines = lines;
    if (use_cuda)
    {
        return;
    }
    for (size_t i = 0; i < correct_lines.size(); ++i)
    {
        correct_lines[i].x1 -= left_padding;
        correct_lines[i].x2 -= left_padding;
        correct_lines[i].y1 -= top_padding;
        correct_lines[i].y2 -= top_padding;
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_zones_list(const zones_cont& zones_)
{
    zones = zones_;
    recalc_correct_zones();
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::get_zones_list(zones_cont& zones_) const
{
    zones_ = zones;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::recalc_correct_zones()
{
    correct_zones = zones;
    if (use_cuda)
    {
        return;
    }
    for (size_t i = 0; i < correct_zones.size(); ++i)
    {
        correct_zones[i].left -= left_padding;
        correct_zones[i].right -= left_padding;
        correct_zones[i].top -= top_padding;
        correct_zones[i].bottom -= top_padding;
    }
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::get_objects(CObjRect* &rect_arr, size_t& rect_count)
{
    rect_count = show_objects? objects_count: 0;
    if (rect_count)
        rect_arr = &obj_rects[0];
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_one_object(unsigned int uid, int left, int right, int top, int bottom)
{
    if (uid)
    {
        for (objects_container::const_iterator iter = objects_history.begin(); iter != objects_history.end(); ++iter)
        {
            if (uid != (*iter)->uid)
                add_uid_to_del_objects((*iter)->uid);
        }
        objects_history.clear();
    }
    else
    {
        uid = 1;
        del_objects_count = 0;
    }

    if (!obj_rects.size())
    {
        CObjRect object(0, 0, 0, 0, 1, 0, 0);
        obj_rects.push_back(object);
    }

    obj_rects[0].left = left;
    obj_rects[0].right = right;
    obj_rects[0].top = top;
    obj_rects[0].bottom = bottom;
    obj_rects[0].uid = uid;
    obj_rects[0].type = unknown_object;
    obj_rects[0].zone_name[0] = '\0';
    obj_rects[0].traectory_size = 1;
    objects_count = 1;


    left_objects_count = 0;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::get_del_objects(unsigned int* &uids_arr, size_t& uids_count)
{
    uids_count = del_objects_count;
    if (uids_count)
        uids_arr = &del_objects[0];
    if (!show_objects)
        del_objects_count = 0;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_object_points(size_t obj_ind, POINTF* points, size_t& max_points)
{
    if (use_cuda || (obj_ind >= obj_rects.size()))
    {
        return false;
    }

    uint32_t left = obj_rects[obj_ind].left - left_padding;
    uint32_t right = obj_rects[obj_ind].right - left_padding;
    uint32_t top = obj_rects[obj_ind].top - top_padding;
    uint32_t bottom = obj_rects[obj_ind].bottom - top_padding;

    mask_type *par = nullptr;
    uint32_t step_x = std::max<uint32_t>(2, (right - left + 1) / 16);
    uint32_t step_y = std::max<uint32_t>(2, (bottom - top + 1) / 16);
    size_t point_ind = 0;
    for (uint32_t y = top + step_y / 2; y < bottom; y += step_y)
    {
        par = &segmentator.get_mask()[(left + step_x / 2) + frame_width * y];
        for (uint32_t x = left + step_x / 2; x < right; x += step_x)
        {
            if (*par)
            {
                if (point_ind >= max_points)
                    break;
                points[point_ind] = POINTF((float)(x + left_padding), (float)(y + top_padding));
                ++point_ind;
            }
            par += step_x;
        }
    }
    max_points = point_ind;
    return true;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::get_left_objects(CLeftObjRect* &rect_arr, size_t& rect_count)
{
    rect_count = (show_objects && show_left_objects) ? left_objects_count : 0;
    if (rect_count)
        rect_arr = &left_obj_rects[0];
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_use_square_segmentation() const
{
    return use_square_segmentation;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_use_square_segmentation(bool use_square_segmentation_)
{
    use_square_segmentation = use_square_segmentation_;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_use_recognition() const
{
    return use_recognition;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_use_recognition(bool new_val)
{
    use_recognition = new_val;
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::get_show_objects() const
{
    return show_objects;
}
////////////////////////////////////////////////////////////////////////////

void CFeinTrack::set_show_objects(bool new_val)
{
    show_objects = new_val;

    // ���� ����������� �������� �� �����, �� ��������� ��������� ������ � ������� ����������
    if (!show_objects)
    {
        frame_width = 0;
        frame_height = 0;

        back_substractor->set_show_objects(show_objects);

        segmentator.set_show_objects(show_objects);

        for (objects_container::const_iterator iter = objects_history.begin(); iter != objects_history.end(); ++iter)
        {
            add_uid_to_del_objects((*iter)->uid);
        }
        objects_history.clear();

        curr_frame = 0;

        objects_count = 0;
        left_objects_count = 0;

        lefted_objects.clear();
    }
}
////////////////////////////////////////////////////////////////////////////

bool CFeinTrack::cut_shadow(CObjectRegion& region)
{
    // ����������� �� �������
    hist_cont horz_hist(region.width(), 0.);

    // ��������� ���������� ��� ������� �������� �������
    build_horz_hist(region, 0, region.height() / 2, horz_hist, frame_width, &segmentator.get_mask()[0]);

    double area1;
    double center1 = calc_center_mass(horz_hist, area1);

    // ��������� ���������� ��� ������ �������� �������
    fill(horz_hist.begin(), horz_hist.end(), 0);
    build_horz_hist(region, region.height() / 2, region.height(), horz_hist, frame_width, &segmentator.get_mask()[0]);

    double area2;
    double center2 = calc_center_mass(horz_hist, area2);

    // �������� ��������� ������������ ������ ����
    if (center1 > center2)
    {
        if (in_range<double>(center1, (2 * region.width()) / 3, region.width()) &&
                in_range<double>(center2, region.width() / 3, (2 * region.width()) / 3))
        {
            // �������� ���� �����
            region.set_left(region.get_left() + region.width() / 2);
            return true;
        }
    }
    else
    {
        if (in_range<double>(center1, 0, region.width() / 3) &&
                in_range<double>(center2, region.width() / 3, (2 * region.width()) / 3))
        {
            // �������� ���� ������
            region.set_right(region.get_left() + region.width() / 2);
            return true;
        }
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////

template<class T>
bool CFeinTrack::is_in_zone(const T &rect, mstring *zone_name) const
{
    if (zone_name)
        *zone_name = "";
    if (correct_zones.empty())
        return true;

    for (size_t i = 0; i < correct_zones.size(); ++i)
    {
        if (correct_zones[i].use_detection &&
                segments_superposition(correct_zones[i].left, correct_zones[i].right, rect.get_left(), rect.get_right()) &&
                segments_superposition(correct_zones[i].top, correct_zones[i].bottom, rect.get_top(), rect.get_bottom()))
        {
            if (zone_name)
                *zone_name = correct_zones[i].name.c_str();
            return true;
        }
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////

template<class T>
const CZone* CFeinTrack::get_zone(const T &rect) const
{
    for (size_t i = 0; i < correct_zones.size(); ++i)
    {
        if (correct_zones[i].use_detection &&
                segments_superposition(correct_zones[i].left, correct_zones[i].right, rect.get_left(), rect.get_right()) &&
                segments_superposition(correct_zones[i].top, correct_zones[i].bottom, rect.get_top(), rect.get_bottom()))
        {
            return &correct_zones[i];
        }
    }
    return nullptr;
}
////////////////////////////////////////////////////////////////////////////

template<class T>
void CFeinTrack::add_object_to_out_rects(const T &rect, const CTrackingObject &object, object_types obj_type, const mstring &zone_name)
{
    int obj_left = rect.get_left();
    int obj_right = rect.get_right();
    int obj_top = rect.get_top();
    int obj_bottom = rect.get_bottom();
    if (analyze_area.left)
    {
        obj_left += left_padding;
        obj_right += left_padding;
    }
    if (analyze_area.top)
    {
        obj_top += top_padding;
        obj_bottom += top_padding;
    }

    // ����� ������� ������� �������� ������ ��������� ������: ������ obj_rects ������� �� �����������
    // �� ����� ������ ����������� ���� ������, ���� �� ����������� ������ ���������� �������� ������, ��� �� ����������
    if (++objects_count > obj_rects.size())
    {
        obj_rects.push_back(CObjRect(obj_left, obj_right, obj_top, obj_bottom, object.uid, object.get_new_center_x(), object.get_new_center_y()));
        if (show_trajectory)
            object.get_traectory(*obj_rects.rbegin(), frame_width, frame_height, left_padding, top_padding);
        obj_rects.rbegin()->type = obj_type;
        obj_rects.rbegin()->zone_name = zone_name;
    }
    else
    {
        obj_rects[objects_count - 1].left = obj_left;
        obj_rects[objects_count - 1].right = obj_right;
        obj_rects[objects_count - 1].top = obj_top;
        obj_rects[objects_count - 1].bottom = obj_bottom;
        obj_rects[objects_count - 1].uid = object.uid;
        obj_rects[objects_count - 1].new_center_x = left_padding + object.get_x_future_val(fps);
        obj_rects[objects_count - 1].new_center_y = top_padding + object.get_y_future_val(fps);
        if (show_trajectory)
            object.get_traectory(obj_rects[objects_count - 1], frame_width, frame_height, left_padding, top_padding);
        else
            obj_rects[objects_count - 1].traectory_size = 1;

        obj_rects[objects_count - 1].type = obj_type;

        obj_rects[objects_count - 1].zone_name = zone_name;
    }
}
////////////////////////////////////////////////////////////////////////////
} //end namespace vl_feintrack
////////////////////////////////////////////////////////////////////////////
