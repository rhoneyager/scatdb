program testF
    use, intrinsic :: ISO_C_BINDING
    implicit none
    interface
        function SDBR_free(ptr) result(res) bind (c, name = "SDBR_free")
            use, intrinsic :: ISO_C_BINDING
            type (c_ptr), value :: ptr
            logical (c_bool) :: res
        end function SDBR_free
        function SDBR_loadDB(fname) result(res) bind (c, name = "SDBR_loadDB")
            use iso_c_binding
            type (c_ptr) :: res
            character ( kind = C_CHAR ) :: fname ( * )
        end function SDBR_loadDB
        function SDBR_writeDB(ptr, fname) result(res) bind (c, name = "SDBR_writeDB")
            use iso_c_binding
            character ( kind = C_CHAR ) :: fname ( * )
            type (c_ptr), value :: ptr
            logical (c_bool) :: res
        end function SDBR_writeDB
    end interface
    type (C_PTR) :: ptr
    logical (c_bool) :: res
    print *, "scatdb_test_f program starting"
    print *, "Skipping SDBR_start"
    print *, "Calling SDBR_loadDB"
    ptr = SDBR_loadDB(C_NULL_CHAR)
    if ( c_associated(ptr) ) then
        print *, "Database loaded successfully"
    else
        print *, "Error loading database"
        stop
    end if
    print *, "Writing to testfor.csv"
    res = SDBR_writeDB(ptr, C_CHAR_"testfor.csv" // C_NULL_CHAR )
    if (res .EQV. .FALSE.) then
        print *, "Error when writing"
        stop
    end if
    print *, "Freeing database"
    res = SDBR_free(ptr)
    if (res .EQV. .FALSE.) then
        print *, "Error freeing database"
        stop
    end if
    print *, "Done"
    stop
end program testF
